#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 1) uniform sampler2D specularOrOrmMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D emissiveMap;
layout(set = 0, binding = 4) uniform sampler2D depthMap;
layout(set = 0, binding = 5) uniform sampler2D lightMap;
layout(set = 0, binding = 6) uniform samplerCube environmentMap;
layout(set = 0, binding = 7) uniform samplerCubeArray reflectionProbes;
layout(set = 0, binding = 8) uniform samplerCubeArray irradianceProbes;
layout(set = 0, binding = 9) uniform samplerCubeArray heroProbes;
layout(set = 0, binding = 10) uniform sampler2D brdfLut;
layout(set = 0, binding = 11) uniform sampler2D lightFunc;
layout(set = 0, binding = 12) uniform sampler2D sceneMap;
layout(set = 0, binding = 13) uniform sampler2D sceneDepthMap;

#define MAX_REFMAP_COUNT 256
#define REF_SAMPLE_COUNT 32

const float M_PI = 3.14159265;
const float GBUFFER_FLAG_SKIP_ATMOS = 0.0;
const float GBUFFER_FLAG_HAS_PBR = 0.67;
const float GBUFFER_FLAG_HAS_HDRI = 1.0;

layout(std140, set = 2, binding = 0) uniform ReflectionProbes
{
    mat4 refBox[MAX_REFMAP_COUNT];
    mat4 heroBox;
    vec4 refSphere[MAX_REFMAP_COUNT];
    vec4 refParams[MAX_REFMAP_COUNT];
    vec4 heroSphere;
    ivec4 refIndex[MAX_REFMAP_COUNT];
    ivec4 refNeighbor[1024];
    ivec4 refBucket[256];
    int refmapCount;
    int heroShape;
    int heroMipCount;
    int heroProbeCount;
} probes;

layout(std140, set = 1, binding = 0) uniform DeferredSoften
{
    mat4 inverse_modelview_delta;
    vec4 atmos_blue_horizon_haze;
    vec4 atmos_blue_density_haze;
    vec4 atmos_density;
    vec4 atmos_glow;
    vec4 atmos_sunlight;
    vec4 atmos_moonlight;
    vec4 atmos_ambient;
    vec4 atmos_lightnorm;
    vec4 ssao_effect_mat0;
    vec4 ssao_effect_mat1;
    vec4 ssao_effect_mat2;
} soften;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 composite_ambient;
    layout(offset = 96) vec4 composite_clip_plane;
    layout(offset = 112) vec4 composite_sun_direction;
    layout(offset = 128) vec4 composite_light;
    layout(offset = 144) vec4 composite_moon_direction;
    layout(offset = 160) vec4 composite_sky_settings;
    layout(offset = 176) vec4 composite_features;
    layout(offset = 192) vec4 composite_light_direction;
    layout(offset = 208) vec4 composite_local_light;
    layout(offset = 224) vec4 composite_ssao;
    layout(offset = 240) vec4 ssr_params0;
    layout(offset = 256) vec4 ssr_params1;
    layout(offset = 272) vec4 composite_environment0;
    layout(offset = 288) vec4 composite_environment1;
    layout(offset = 304) vec4 composite_environment2;
    layout(offset = 352) mat4 inverse_projection;
    layout(offset = 416) vec4 scene_reflection;
    layout(offset = 432) vec4 scene_direct;
} pc;

vec3 srgb_to_linear(vec3 color)
{
    bvec3 cutoff = lessThanEqual(color, vec3(0.04045));
    vec3 low = color / 12.92;
    vec3 high = pow((color + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
}

vec3 linear_to_srgb(vec3 color)
{
    color = max(color, vec3(0.0));
    bvec3 cutoff = lessThanEqual(color, vec3(0.0031308));
    vec3 low = color * 12.92;
    vec3 high = pow(color, vec3(1.0 / 2.4)) * 1.055 - vec3(0.055);
    return mix(high, low, cutoff);
}

vec3 clamp_hdr_range(vec3 color)
{
    color = mix(color, vec3(1.0), isinf(color));
    color = mix(color, vec3(0.0), isnan(color));
    return clamp(color, vec3(0.0), vec3(11.2));
}

vec3 decode_gbuffer_normal(vec4 encoded)
{
    vec2 fenc = encoded.xy * 4.0 - 2.0;
    float f = dot(fenc, fenc);
    float g = sqrt(max(1.0 - f / 4.0, 0.0));
    vec3 normal = vec3(fenc * g, 1.0 - f / 2.0);
    if (dot(normal, normal) <= 0.0001)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    return normalize(normal);
}

bool get_gbuffer_flag(float data, float flag)
{
    return abs(data - flag) < 0.1;
}

vec3 fallback_sky_color(vec2 texcoord)
{
    float horizon = smoothstep(0.0, 1.0, texcoord.y);
    vec3 ambient_color = max(pc.composite_ambient.rgb, vec3(0.0));
    vec3 direct_color = max(pc.composite_light.rgb, vec3(0.0));
    float sky_hdr_scale =
        pc.scene_reflection.w > 0.5 ?
            max(pc.composite_sky_settings.y, 1.0) :
            1.0;
    vec3 horizon_color = ambient_color * 1.15 + direct_color * 0.08;
    vec3 zenith_color = ambient_color * 0.72 + direct_color * 0.24;
    return max(mix(horizon_color, zenith_color, horizon) * sky_hdr_scale, vec3(0.0));
}

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 approximate_view_direction(vec2 texcoord)
{
    vec2 dimensions = max(vec2(textureSize(diffuseMap, 0)), vec2(1.0));
    float aspect = dimensions.x / dimensions.y;
    vec2 ndc = texcoord * 2.0 - 1.0;
    ndc.y = -ndc.y;
    ndc.x *= aspect;
    return normalize(vec3(-ndc, 1.0));
}

vec3 sample_environment(vec3 normal, vec3 view_dir, float roughness, float probe_ambiance)
{
    if (pc.scene_reflection.y < 0.5 || probe_ambiance <= 0.0)
    {
        return vec3(0.0);
    }

    vec3 reflected = reflect(-view_dir, normal);
    vec3 env_dir = mat3(
        pc.composite_environment0.xyz,
        pc.composite_environment1.xyz,
        pc.composite_environment2.xyz) * reflected;
    vec3 cube_color = textureLod(
        environmentMap,
        normalize(env_dir),
        clamp(roughness * pc.composite_sky_settings.w, 0.0, pc.composite_sky_settings.w)).rgb;
    return max(cube_color, vec3(0.0)) * max(probe_ambiance, 0.0);
}

void calc_diffuse_specular(vec3 base_color, float metallic, out vec3 diffuse_color, out vec3 specular_color)
{
    vec3 f0 = vec3(0.04);
    diffuse_color = base_color * (vec3(1.0) - f0);
    diffuse_color *= 1.0 - metallic;
    specular_color = mix(f0, base_color, metallic);
}

vec2 brdf_lookup(float ndotv, float roughness)
{
    return texture(brdfLut, vec2(clamp(ndotv, 0.0, 1.0), roughness)).rg;
}

void pbr_ibl(
    vec3 diffuse_color,
    vec3 specular_color,
    vec3 radiance,
    vec3 irradiance,
    float ao,
    float ndotv,
    float perceptual_roughness,
    out vec3 diffuse_out,
    out vec3 specular_out)
{
    vec2 brdf = brdf_lookup(ndotv, 1.0 - perceptual_roughness);
    diffuse_out = irradiance * diffuse_color * ao;
    specular_out = radiance * (specular_color * brdf.x + brdf.y) * ao;
}

struct PBRInfo
{
    float NdotL;
    float NdotV;
    float NdotH;
    float LdotH;
    float VdotH;
    float perceptualRoughness;
    float metalness;
    vec3 reflectance0;
    vec3 reflectance90;
    float alphaRoughness;
    vec3 diffuseColor;
    vec3 specularColor;
};

vec3 diffuse_brdf(PBRInfo pbr_inputs)
{
    return pbr_inputs.diffuseColor / M_PI;
}

vec3 specular_reflection(PBRInfo pbr_inputs)
{
    return pbr_inputs.reflectance0 +
        (pbr_inputs.reflectance90 - pbr_inputs.reflectance0) *
        pow(clamp(1.0 - pbr_inputs.VdotH, 0.0, 1.0), 5.0);
}

float geometric_occlusion(PBRInfo pbr_inputs)
{
    float ndotl = pbr_inputs.NdotL;
    float ndotv = pbr_inputs.NdotV;
    float r = pbr_inputs.alphaRoughness;
    float attenuation_l =
        2.0 * ndotl / (ndotl + sqrt(r * r + (1.0 - r * r) * (ndotl * ndotl)));
    float attenuation_v =
        2.0 * ndotv / (ndotv + sqrt(r * r + (1.0 - r * r) * (ndotv * ndotv)));
    return attenuation_l * attenuation_v;
}

float microfacet_distribution(PBRInfo pbr_inputs)
{
    float roughness_sq = pbr_inputs.alphaRoughness * pbr_inputs.alphaRoughness;
    float f =
        (pbr_inputs.NdotH * roughness_sq - pbr_inputs.NdotH) *
            pbr_inputs.NdotH +
        1.0;
    return roughness_sq / (M_PI * f * f);
}

void pbr_punctual(
    vec3 diffuse_color,
    vec3 specular_color,
    float perceptual_roughness,
    float metallic,
    vec3 n,
    vec3 v,
    vec3 l,
    out float nl,
    out vec3 diff,
    out vec3 spec)
{
    perceptual_roughness = max(perceptual_roughness, 8.0 / 255.0);

    float alpha_roughness = perceptual_roughness * perceptual_roughness;
    float reflectance = max(max(specular_color.r, specular_color.g), specular_color.b);
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 h = normalize(l + v);

    float ndotl = clamp(dot(n, l), 0.001, 1.0);
    float ndotv = clamp(abs(dot(n, v)), 0.001, 1.0);
    float ndoth = clamp(dot(n, h), 0.0, 1.0);
    float ldoth = clamp(dot(l, h), 0.0, 1.0);
    float vdoth = clamp(dot(v, h), 0.0, 1.0);

    PBRInfo pbr_inputs = PBRInfo(
        ndotl,
        ndotv,
        ndoth,
        ldoth,
        vdoth,
        perceptual_roughness,
        metallic,
        specular_color.rgb,
        vec3(1.0) * reflectance90,
        alpha_roughness,
        diffuse_color,
        specular_color);

    vec3 F = specular_reflection(pbr_inputs);
    float G = geometric_occlusion(pbr_inputs);
    float D = microfacet_distribution(pbr_inputs);

    diff = (1.0 - F) * diffuse_brdf(pbr_inputs);
    spec = F * G * D / (4.0 * ndotl * ndotv);
    nl = ndotl;
}

void adjust_irradiance(inout vec3 irradiance, float ambient_occlusion)
{
    float scale = max(pc.scene_direct.z, 0.0);
    float max_value = max(pc.scene_direct.w, 0.0);
    if (pc.composite_features.y > 0.5 && scale > 0.0 && max_value > 0.0)
    {
        mat3 ssao_effect_mat =
            mat3(
                soften.ssao_effect_mat0.xyz,
                soften.ssao_effect_mat1.xyz,
                soften.ssao_effect_mat2.xyz);
        vec3 ssao_irradiance =
            ssao_effect_mat * min(irradiance * scale, vec3(max_value));
        irradiance = mix(ssao_irradiance, irradiance, ambient_occlusion);
    }
}

vec3 pbr_base_light(
    vec3 diffuse_color,
    vec3 specular_color,
    float metallic,
    vec3 v,
    vec3 norm,
    float perceptual_roughness,
    vec3 light_dir,
    vec3 sunlit,
    float sun_shadow,
    vec3 radiance,
    vec3 irradiance,
    vec3 color_emissive,
    float ao,
    bool classic_mode)
{
    vec3 color = vec3(0.0);
    float ndotv = clamp(abs(dot(norm, v)), 0.001, 1.0);
    vec3 ibl_diff = vec3(0.0);
    vec3 ibl_spec = vec3(0.0);
    pbr_ibl(
        diffuse_color,
        specular_color,
        radiance,
        irradiance,
        ao,
        ndotv,
        perceptual_roughness,
        ibl_diff,
        ibl_spec);

    color += ibl_diff;

    float nl = 0.0;
    vec3 diff_punc = vec3(0.0);
    vec3 spec_punc = vec3(0.0);
    pbr_punctual(
        diffuse_color,
        specular_color,
        perceptual_roughness,
        metallic,
        norm,
        v,
        normalize(light_dir),
        nl,
        diff_punc,
        spec_punc);

    if (classic_mode)
    {
        irradiance = srgb_to_linear(irradiance * 0.9);
        float da = pow(nl, 1.2);
        vec3 sun_contrib = vec3(min(da, sun_shadow));
        sun_contrib =
            srgb_to_linear(linear_to_srgb(sun_contrib) * sunlit * 0.7) * M_PI;
        vec3 final_ambient = irradiance * diffuse_color;
        vec3 final_sun =
            clamp(
                sun_contrib * ((diff_punc + spec_punc) * sun_shadow),
                vec3(0.0),
                vec3(10.0));
        color =
            srgb_to_linear(
                linear_to_srgb(final_ambient) +
                (linear_to_srgb(final_sun) * 1.1));
    }
    else
    {
        color +=
            clamp(nl * (diff_punc + spec_punc), vec3(0.0), vec3(10.0)) *
            sunlit *
            3.0 *
            sun_shadow;
    }

    color += ibl_spec;
    color += color_emissive;
    return color;
}

void calc_half_vectors(
    vec3 lv,
    vec3 n,
    vec3 v,
    out vec3 h,
    out vec3 l,
    out float nh,
    out float nl,
    out float nv,
    out float vh,
    out float light_dist)
{
    l = normalize(lv);
    h = normalize(l + v);

    float eps = 0.000001;
    nh = clamp(dot(n, h), eps, 1.0);
    nl = clamp(dot(n, l), eps, 1.0);
    nv = clamp(dot(n, v), eps, 1.0);
    vh = clamp(dot(v, h), eps, 1.0);
    light_dist = length(lv);
}

void apply_gloss_env(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm)
{
    glossenv *= 0.5;
    float fresnel = clamp(1.0 + dot(normalize(pos), norm), 0.3, 1.0);
    fresnel *= fresnel;
    fresnel *= spec.a;
    glossenv *= spec.rgb * fresnel;
    glossenv *= vec3(1.0) - color;
    color += glossenv * 0.5;
}

void apply_legacy_env(
    inout vec3 color,
    vec3 legacyenv,
    vec4 spec,
    vec3 pos,
    vec3 norm,
    float env_intensity)
{
    vec3 reflected_color = legacyenv;
    vec3 look_at = normalize(pos);
    float fresnel = 1.0 + dot(look_at, norm);
    fresnel *= fresnel;
    fresnel = min(fresnel + env_intensity, 1.0);
    reflected_color *= env_intensity * fresnel;
    color = mix(color, reflected_color * 0.5, env_intensity);
}

int probeIndex[REF_SAMPLE_COUNT];
int probeInfluences = 0;
bool sampleAutomaticProbes = true;

vec3 safe_normalize(vec3 value)
{
    float len2 = dot(value, value);
    if (len2 <= 0.000001)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    return value * inversesqrt(len2);
}

float ambient_lighting(vec3 norm, vec3 light_dir)
{
    float ambient = min(abs(dot(norm.xyz, light_dir.xyz)), 1.0);
    ambient *= 0.5;
    ambient *= ambient;
    return 1.0 - ambient;
}

void calc_atmospheric_vars(
    vec3 in_position_eye,
    vec3 light_dir,
    out vec3 sunlit,
    out vec3 amblit,
    out vec3 additive,
    out vec3 atten)
{
    vec3 rel_pos = in_position_eye;
    float max_y = max(soften.atmos_density.w, 0.000001);
    if (abs(rel_pos.y) > max_y)
    {
        rel_pos *= max_y / rel_pos.y;
    }

    vec3 rel_pos_norm = safe_normalize(rel_pos);
    float rel_pos_len = length(rel_pos);
    vec3 lightnorm = soften.atmos_lightnorm.xyz;
    if (dot(lightnorm, lightnorm) <= 0.000001)
    {
        lightnorm = light_dir;
    }

    vec3 sunlight =
        pc.composite_sun_direction.w > 0.5 ?
            soften.atmos_sunlight.rgb :
            soften.atmos_moonlight.rgb;
    vec3 blue_horizon = soften.atmos_blue_horizon_haze.rgb;
    vec3 blue_density = soften.atmos_blue_density_haze.rgb;
    float haze_horizon = soften.atmos_blue_horizon_haze.a;
    float haze_density = soften.atmos_blue_density_haze.a;
    float cloud_shadow = clamp(soften.atmos_density.x, 0.0, 1.0);
    float density_multiplier = max(soften.atmos_density.y, 0.0);
    float distance_multiplier = max(soften.atmos_density.z, 0.0);

    vec3 light_atten =
        (blue_density + vec3(haze_density * 0.25)) *
        (density_multiplier * max_y);
    vec3 combined_haze = max(blue_density + vec3(haze_density), vec3(0.000001));
    vec3 blue_weight = blue_density / combined_haze;
    vec3 haze_weight = vec3(haze_density) / combined_haze;

    float above_horizon_factor = 1.0 / max(0.000001, lightnorm.y);
    sunlight *= exp(-light_atten * above_horizon_factor);

    float density_dist = rel_pos_len * density_multiplier;
    combined_haze =
        exp(-combined_haze * density_dist * distance_multiplier);
    atten = combined_haze.rgb;

    float haze_glow = dot(rel_pos_norm, lightnorm.xyz);
    haze_glow *= max(0.0, dot(light_dir, rel_pos_norm));
    haze_glow = 1.0 - haze_glow;
    haze_glow = max(haze_glow, 0.001);
    haze_glow *= soften.atmos_glow.x;
    haze_glow =
        clamp(pow(haze_glow, soften.atmos_glow.z), -100000.0, 100000.0);
    haze_glow += 0.25;
    haze_glow *= soften.atmos_glow.w;

    vec3 ambient_color = soften.atmos_ambient.rgb;
    vec3 tmp_ambient =
        ambient_color + (vec3(1.0) - ambient_color) * cloud_shadow * 0.5;
    vec3 cs = sunlight.rgb * (1.0 - cloud_shadow);
    additive =
        (blue_horizon.rgb * blue_weight.rgb) * (cs + tmp_ambient.rgb) +
        (haze_horizon * haze_weight.rgb) *
            (cs * haze_glow + tmp_ambient.rgb);

    sunlit = sunlight.rgb;
    amblit = pow(tmp_ambient.rgb, vec3(0.9)) * 0.57;
    additive *= vec3(1.0 - combined_haze);
    additive = min(additive, vec3(10.0));
}

void calc_atmospheric_vars_linear(
    vec3 in_position_eye,
    vec3 norm,
    vec3 light_dir,
    out vec3 sunlit,
    out vec3 amblit,
    out vec3 additive,
    out vec3 atten)
{
    calc_atmospheric_vars(
        in_position_eye,
        light_dir,
        sunlit,
        amblit,
        additive,
        atten);

    amblit *= ambient_lighting(norm, light_dir);

    if (pc.composite_moon_direction.w < 0.5)
    {
        amblit = srgb_to_linear(amblit);
        amblit = vec3(dot(amblit, vec3(0.2126, 0.7152, 0.0722)));
        sunlit = srgb_to_linear(sunlit);
    }

    sunlit *= soften.atmos_sunlight.a;
    amblit *= soften.atmos_moonlight.a;
}

vec3 atmos_frag_lighting_linear(vec3 light, vec3 additive, vec3 atten)
{
    light *= atten.r;
    additive = srgb_to_linear(additive * 2.0);
    additive *= pc.composite_sky_settings.y;
    return light + additive;
}

bool has_reflection_probe_inputs()
{
    return pc.scene_reflection.y > 0.5 &&
        probes.refmapCount > 0 &&
        textureSize(reflectionProbes, 0).z > 0 &&
        textureSize(irradianceProbes, 0).z > 0;
}

bool should_sample_probe(int i, vec3 pos)
{
    if (i < 0 || i >= probes.refmapCount || i >= MAX_REFMAP_COUNT)
    {
        return false;
    }

    if (probes.refIndex[i].w < 0)
    {
        vec4 v = probes.refBox[i] * vec4(pos, 1.0);
        if (abs(v.x) > 1.0 ||
            abs(v.y) > 1.0 ||
            abs(v.z) > 1.0)
        {
            return false;
        }

        sampleAutomaticProbes = false;
    }
    else
    {
        if (probes.refIndex[i].w == 0 && !sampleAutomaticProbes)
        {
            return false;
        }

        vec3 delta = pos - probes.refSphere[i].xyz;
        float radius = probes.refSphere[i].w;
        if (dot(delta, delta) > radius * radius)
        {
            return false;
        }
    }

    return true;
}

int get_probe_start_index(vec3 pos)
{
    int bucket = clamp(int(floor(-pos.z)), 0, 255);
    return clamp(probes.refBucket[bucket].x, 1, probes.refmapCount + 1);
}

void append_probe_index(int index)
{
    if (probeInfluences < REF_SAMPLE_COUNT)
    {
        probeIndex[probeInfluences] = index;
        ++probeInfluences;
    }
}

void pre_probe_sample(vec3 pos)
{
    probeInfluences = 0;
    sampleAutomaticProbes = true;

    int start = get_probe_start_index(pos);
    for (int i = start; i < probes.refmapCount && probeInfluences < REF_SAMPLE_COUNT; ++i)
    {
        if (!should_sample_probe(i, pos))
        {
            continue;
        }

        append_probe_index(i);

        int neighbor_idx = probes.refIndex[i].y;
        if (neighbor_idx == -1)
        {
            continue;
        }

        int neighbor_count = probes.refIndex[i].z;
        int count = 0;
        while (count < neighbor_count &&
               neighbor_idx >= 0 &&
               neighbor_idx < 1024 &&
               probeInfluences < REF_SAMPLE_COUNT)
        {
            ivec4 neighbors = probes.refNeighbor[neighbor_idx];
            for (int component = 0;
                 component < 4 &&
                    count < neighbor_count &&
                    probeInfluences < REF_SAMPLE_COUNT;
                 ++component)
            {
                int idx = component == 0 ? neighbors.x :
                    (component == 1 ? neighbors.y :
                    (component == 2 ? neighbors.z : neighbors.w));
                if (should_sample_probe(idx, pos))
                {
                    append_probe_index(idx);
                }
                ++count;
            }
            ++neighbor_idx;
        }

        break;
    }

    if (sampleAutomaticProbes)
    {
        append_probe_index(0);
    }
}

vec3 sphere_intersect(vec3 origin, vec3 dir, vec3 center, float radius2)
{
    vec3 l = center - origin;
    float tca = dot(l, dir);
    float d2 = max(dot(l, l) - tca * tca, 0.0);
    float thc = sqrt(max(radius2 - d2, 0.0));
    return origin + dir * (tca + thc);
}

vec3 box_intersect(vec3 origin, vec3 dir, mat4 clip_to_local, out float d, float scale)
{
    vec3 ray_ls = mat3(clip_to_local) * dir;
    vec3 position_ls = (clip_to_local * vec4(origin, 1.0)).xyz;

    d = 1.0 - max(max(abs(position_ls.x), abs(position_ls.y)), abs(position_ls.z));

    vec3 unitary = vec3(scale);
    vec3 safe_ray = vec3(
        abs(ray_ls.x) > 0.000001 ? ray_ls.x : (ray_ls.x < 0.0 ? -0.000001 : 0.000001),
        abs(ray_ls.y) > 0.000001 ? ray_ls.y : (ray_ls.y < 0.0 ? -0.000001 : 0.000001),
        abs(ray_ls.z) > 0.000001 ? ray_ls.z : (ray_ls.z < 0.0 ? -0.000001 : 0.000001));
    vec3 first_plane_intersect = (unitary - position_ls) / safe_ray;
    vec3 second_plane_intersect = (-unitary - position_ls) / safe_ray;
    vec3 furthest_plane = max(first_plane_intersect, second_plane_intersect);
    float distance_to_box =
        min(furthest_plane.x, min(furthest_plane.y, furthest_plane.z));

    return origin + dir * distance_to_box;
}

float sphere_weight(vec3 pos, vec3 dir, vec3 origin, float radius, vec4 params, out float dw)
{
    float inner_radius = radius * 0.5;
    vec3 delta = pos - origin;
    float distance_to_probe = max(length(delta), 0.001);

    float attenuation =
        1.0 - max(distance_to_probe - inner_radius, 0.0) /
            max(radius - inner_radius, 0.001);
    float w = 1.0 / distance_to_probe;

    w *= params.z;
    dw = w * attenuation * max(radius, 1.0) * 4.0;
    return w * attenuation;
}

int reflection_probe_layer(int i)
{
    int layer_count = textureSize(reflectionProbes, 0).z;
    if (layer_count <= 0)
    {
        return -1;
    }
    return clamp(probes.refIndex[i].x, 0, layer_count - 1);
}

int irradiance_probe_layer(int i)
{
    int layer_count = textureSize(irradianceProbes, 0).z;
    if (layer_count <= 0)
    {
        return -1;
    }
    return clamp(probes.refIndex[i].x, 0, layer_count - 1);
}

vec3 tap_reflection_map(
    vec3 pos,
    vec3 dir,
    out float w,
    out float dw,
    float lod,
    int i)
{
    w = 0.0;
    dw = 0.0;

    int layer = reflection_probe_layer(i);
    if (layer < 0)
    {
        return vec3(0.0);
    }

    vec3 v;
    if (probes.refIndex[i].w < 0)
    {
        float distance_to_box = 0.0;
        v = box_intersect(pos, dir, probes.refBox[i], distance_to_box, 1.0);
        w = max(distance_to_box, 0.001);
        dw = w;
    }
    else
    {
        float radius = probes.refSphere[i].w;
        float radius2 =
            probes.refIndex[i].w < 1 ?
                4096.0 * 4096.0 :
                radius * radius;
        v = sphere_intersect(pos, dir, probes.refSphere[i].xyz, radius2);
        w = sphere_weight(pos, dir, probes.refSphere[i].xyz, radius, probes.refParams[i], dw);
    }

    vec3 sample_dir = mat3(
        pc.composite_environment0.xyz,
        pc.composite_environment1.xyz,
        pc.composite_environment2.xyz) *
        (v - probes.refSphere[i].xyz);
    return textureLod(
        reflectionProbes,
        vec4(safe_normalize(sample_dir), layer),
        lod).rgb * max(probes.refParams[i].y, 0.0);
}

vec3 tap_irradiance_map(
    vec3 pos,
    vec3 dir,
    out float w,
    out float dw,
    int i,
    vec3 fallback_ambient)
{
    w = 0.0;
    dw = 0.0;

    int layer = irradiance_probe_layer(i);
    if (layer < 0)
    {
        return fallback_ambient;
    }

    vec3 v;
    if (probes.refIndex[i].w < 0)
    {
        float distance_to_box = 0.0;
        v = box_intersect(pos, dir, probes.refBox[i], distance_to_box, 3.0);
        w = max(distance_to_box, 0.001);
        dw = w;
    }
    else
    {
        float radius = probes.refSphere[i].w;
        float radius2 =
            probes.refIndex[i].w < 1 ?
                4096.0 * 4096.0 :
                radius * radius;
        v = sphere_intersect(pos, dir, probes.refSphere[i].xyz, radius2);
        w = sphere_weight(pos, dir, probes.refSphere[i].xyz, radius, probes.refParams[i], dw);
    }

    vec3 sample_dir = mat3(
        pc.composite_environment0.xyz,
        pc.composite_environment1.xyz,
        pc.composite_environment2.xyz) *
        (v - probes.refSphere[i].xyz);
    vec3 col =
        textureLod(irradianceProbes, vec4(safe_normalize(sample_dir), layer), 0.0).rgb *
        max(probes.refParams[i].x, 0.0);

    return mix(fallback_ambient, col, min(max(probes.refParams[i].x, 0.0), 1.0));
}

vec3 sample_probe_radiance(vec3 pos, vec3 dir, float lod)
{
    float weight_auto = 0.0;
    float weight_manual = 0.0;
    float distance_weight_auto = 0.0;
    float distance_weight_manual = 0.0;
    vec3 color_auto = vec3(0.0);
    vec3 color_manual = vec3(0.0);

    for (int idx = 0; idx < probeInfluences; ++idx)
    {
        int i = probeIndex[idx];
        int probe_type = clamp(abs(probes.refIndex[i].w), 0, 1);
        if (probe_type == 0 && !sampleAutomaticProbes)
        {
            continue;
        }

        float w = 0.0;
        float dw = 0.0;
        vec3 probe_color = tap_reflection_map(pos, dir, w, dw, lod, i);
        if (probe_type == 0)
        {
            color_auto += probe_color * w;
            weight_auto += w;
            distance_weight_auto += dw;
        }
        else
        {
            color_manual += probe_color * w;
            weight_manual += w;
            distance_weight_manual += dw;
        }
    }

    if (sampleAutomaticProbes && weight_auto > 0.0)
    {
        color_auto /= weight_auto;
        if (weight_manual > 0.0)
        {
            color_manual /= weight_manual;
            color_manual =
                mix(color_auto, color_manual, min(distance_weight_manual, 1.0));
            color_auto = vec3(0.0);
        }
    }
    else if (weight_manual > 0.0)
    {
        color_manual /= weight_manual;
        color_auto = vec3(0.0);
    }

    return color_manual + color_auto;
}

vec3 sample_probe_ambient(vec3 pos, vec3 dir, vec3 fallback_ambient)
{
    float weight_auto = 0.0;
    float weight_manual = 0.0;
    float distance_weight_auto = 0.0;
    float distance_weight_manual = 0.0;
    vec3 color_auto = vec3(0.0);
    vec3 color_manual = vec3(0.0);

    for (int idx = 0; idx < probeInfluences; ++idx)
    {
        int i = probeIndex[idx];
        int probe_type = clamp(abs(probes.refIndex[i].w), 0, 1);
        if (probe_type == 0 && !sampleAutomaticProbes)
        {
            continue;
        }

        float w = 0.0;
        float dw = 0.0;
        vec3 probe_color =
            tap_irradiance_map(pos, dir, w, dw, i, fallback_ambient);
        if (probe_type == 0)
        {
            color_auto += probe_color * w;
            weight_auto += w;
            distance_weight_auto += dw;
        }
        else
        {
            color_manual += probe_color * w;
            weight_manual += w;
            distance_weight_manual += dw;
        }
    }

    if (sampleAutomaticProbes && weight_auto > 0.0)
    {
        color_auto /= weight_auto;
        if (weight_manual > 0.0)
        {
            color_manual /= weight_manual;
            color_manual =
                mix(color_auto, color_manual, min(distance_weight_manual, 1.0));
            color_auto = vec3(0.0);
        }
    }
    else if (weight_manual > 0.0)
    {
        color_manual /= weight_manual;
        color_auto = vec3(0.0);
    }

    vec3 result = color_manual + color_auto;
    return max(result, fallback_ambient);
}

void tap_hero_probe(inout vec3 glossenv, vec3 pos, vec3 norm, float glossiness)
{
    if (probes.heroProbeCount <= 0 || textureSize(heroProbes, 0).z <= 0)
    {
        return;
    }

    float clip_dist = dot(pos, pc.composite_clip_plane.xyz) + pc.composite_clip_plane.w;
    float w = 0.0;
    float dw = 0.0;
    const float falloff_mult = 10.0;
    vec3 reflected = reflect(pos, norm);
    if (probes.heroShape < 1)
    {
        float distance_to_box = 0.0;
        box_intersect(pos, norm, probes.heroBox, distance_to_box, 1.0);
        w = max(distance_to_box, 0.0);
    }
    else
    {
        w = sphere_weight(
            pos,
            reflected,
            probes.heroSphere.xyz,
            probes.heroSphere.w,
            vec4(1.0),
            dw);
    }

    clip_dist = clip_dist * 0.95 + 0.05;
    clip_dist = clamp(clip_dist * falloff_mult, 0.0, 1.0);
    w = clamp(w * falloff_mult * clip_dist, 0.0, 1.0);
    w = mix(0.0, w, clamp(glossiness - 0.75, 0.0, 1.0) * 4.0);

    float hero_lod =
        (1.0 - glossiness) * max(float(probes.heroMipCount), 0.0);
    vec3 hero_dir = mat3(
        pc.composite_environment0.xyz,
        pc.composite_environment1.xyz,
        pc.composite_environment2.xyz) * reflected;
    glossenv = mix(
        glossenv,
        textureLod(heroProbes, vec4(safe_normalize(hero_dir), 0), hero_lod).rgb,
        w);
}

vec3 reconstruct_view_position(vec2 texcoord, float depth);

const vec3 POISSON3D_SAMPLES[128] = vec3[128](
    vec3(0.5433144, 0.1122154, 0.2501391),
    vec3(0.6575254, 0.721409, 0.16286),
    vec3(0.02888453, 0.05170321, 0.7573566),
    vec3(0.06635678, 0.8286457, 0.07157445),
    vec3(0.8957489, 0.4005505, 0.7916042),
    vec3(0.3423355, 0.5053263, 0.9193521),
    vec3(0.9694794, 0.9461077, 0.5406441),
    vec3(0.9975473, 0.02789414, 0.7320132),
    vec3(0.07781899, 0.3862341, 0.918594),
    vec3(0.4439073, 0.9686955, 0.4055861),
    vec3(0.9657035, 0.6624081, 0.7082613),
    vec3(0.7712346, 0.07273269, 0.3292839),
    vec3(0.2489169, 0.2550394, 0.1950516),
    vec3(0.7249326, 0.9328285, 0.3352458),
    vec3(0.6028461, 0.4424961, 0.5393377),
    vec3(0.2879795, 0.7427881, 0.6619173),
    vec3(0.3193627, 0.0486145, 0.08109283),
    vec3(0.1233155, 0.602641, 0.4378719),
    vec3(0.9800708, 0.211729, 0.6771586),
    vec3(0.4894537, 0.3319927, 0.8087631),
    vec3(0.4802743, 0.6358885, 0.814935),
    vec3(0.2692913, 0.9911493, 0.9934899),
    vec3(0.5648789, 0.8553897, 0.7784553),
    vec3(0.8497344, 0.7870212, 0.02065313),
    vec3(0.7503014, 0.2826185, 0.05412734),
    vec3(0.8045461, 0.6167251, 0.9532926),
    vec3(0.04225039, 0.2141281, 0.8678675),
    vec3(0.07116079, 0.9971236, 0.3396397),
    vec3(0.464099, 0.480959, 0.2775862),
    vec3(0.6346927, 0.31871, 0.6588384),
    vec3(0.449012, 0.8189669, 0.2736875),
    vec3(0.452929, 0.2119148, 0.672004),
    vec3(0.01506042, 0.7102436, 0.9800494),
    vec3(0.1970513, 0.4713539, 0.4644522),
    vec3(0.13715, 0.7253224, 0.5056525),
    vec3(0.9006432, 0.5335414, 0.02206874),
    vec3(0.9960898, 0.7961011, 0.01468861),
    vec3(0.3386469, 0.6337739, 0.9310676),
    vec3(0.1745718, 0.9114985, 0.1728188),
    vec3(0.6342545, 0.5721557, 0.4553517),
    vec3(0.1347412, 0.1137158, 0.7793725),
    vec3(0.3574478, 0.3448052, 0.08741581),
    vec3(0.7283059, 0.4753885, 0.2240275),
    vec3(0.8293507, 0.9971212, 0.2747005),
    vec3(0.6501846, 0.000688076, 0.7795712),
    vec3(0.01149416, 0.4930083, 0.792608),
    vec3(0.666189, 0.1875442, 0.7256873),
    vec3(0.8538797, 0.2107637, 0.1547532),
    vec3(0.5826825, 0.9750752, 0.9105834),
    vec3(0.8914346, 0.08266425, 0.5484225),
    vec3(0.4374518, 0.02987111, 0.7810078),
    vec3(0.2287418, 0.1443802, 0.1176908),
    vec3(0.2671157, 0.8929081, 0.8989366),
    vec3(0.5425819, 0.5524959, 0.6963879),
    vec3(0.3515188, 0.8304397, 0.0502702),
    vec3(0.3354864, 0.2130747, 0.141169),
    vec3(0.9729427, 0.3509927, 0.6098799),
    vec3(0.7585629, 0.7115368, 0.9099342),
    vec3(0.0140543, 0.6072157, 0.9436461),
    vec3(0.9190664, 0.8497264, 0.1643751),
    vec3(0.1538157, 0.3219983, 0.2984214),
    vec3(0.8854713, 0.2968667, 0.8511457),
    vec3(0.1910622, 0.03047311, 0.3571215),
    vec3(0.2456353, 0.5568692, 0.3530164),
    vec3(0.6927255, 0.8073994, 0.5808484),
    vec3(0.8089353, 0.8969175, 0.3427134),
    vec3(0.194477, 0.7985603, 0.8712182),
    vec3(0.7256182, 0.5653068, 0.3985921),
    vec3(0.9889427, 0.4584851, 0.8363391),
    vec3(0.5718582, 0.2127113, 0.2950557),
    vec3(0.5480209, 0.0193435, 0.2992659),
    vec3(0.6598953, 0.09478426, 0.92187),
    vec3(0.1385615, 0.2193868, 0.205245),
    vec3(0.7623423, 0.1790726, 0.1508465),
    vec3(0.7569032, 0.3773386, 0.4393887),
    vec3(0.5842971, 0.6538072, 0.5224424),
    vec3(0.9954313, 0.5763943, 0.9169143),
    vec3(0.001311183, 0.340363, 0.1488652),
    vec3(0.8167927, 0.4947158, 0.4454727),
    vec3(0.3978434, 0.7106082, 0.002727509),
    vec3(0.5459411, 0.7473233, 0.7062873),
    vec3(0.4151598, 0.5614617, 0.4748358),
    vec3(0.4440694, 0.1195122, 0.9624678),
    vec3(0.1081301, 0.4813806, 0.07047641),
    vec3(0.2402785, 0.3633997, 0.3898734),
    vec3(0.2317942, 0.6488295, 0.4221864),
    vec3(0.01145542, 0.9304277, 0.4105759),
    vec3(0.3563728, 0.9228861, 0.3282344),
    vec3(0.855314, 0.6949819, 0.3175117),
    vec3(0.730832, 0.01478493, 0.5728671),
    vec3(0.9304829, 0.02653277, 0.712552),
    vec3(0.4132186, 0.4127623, 0.6084146),
    vec3(0.7517329, 0.9978395, 0.1330464),
    vec3(0.5210338, 0.4318751, 0.9721575),
    vec3(0.02953994, 0.1375937, 0.9458942),
    vec3(0.1835506, 0.9896691, 0.7919457),
    vec3(0.3857062, 0.2682322, 0.1264563),
    vec3(0.6319699, 0.8735335, 0.04390657),
    vec3(0.5630485, 0.3339024, 0.993995),
    vec3(0.90701, 0.1512893, 0.8970422),
    vec3(0.3027443, 0.1144253, 0.1488708),
    vec3(0.9149003, 0.7382028, 0.7914025),
    vec3(0.07979286, 0.6892691, 0.2866171),
    vec3(0.7743186, 0.8046008, 0.4399814),
    vec3(0.3128662, 0.4362317, 0.6030678),
    vec3(0.1133721, 0.01605821, 0.391872),
    vec3(0.5185481, 0.9210006, 0.7889017),
    vec3(0.8217013, 0.325305, 0.1668191),
    vec3(0.8358996, 0.1449739, 0.3668382),
    vec3(0.1778213, 0.5599256, 0.1327691),
    vec3(0.06690693, 0.5508637, 0.07212365),
    vec3(0.9750564, 0.284066, 0.5727578),
    vec3(0.4350255, 0.8949825, 0.03574753),
    vec3(0.8931149, 0.9177974, 0.8123496),
    vec3(0.9055127, 0.989903, 0.813235),
    vec3(0.2897243, 0.3123978, 0.5083504),
    vec3(0.1519223, 0.3958645, 0.2640327),
    vec3(0.6840154, 0.6463035, 0.2346607),
    vec3(0.986473, 0.8714055, 0.3960275),
    vec3(0.6819352, 0.4169535, 0.8379834),
    vec3(0.9147297, 0.6144146, 0.7313942),
    vec3(0.6554981, 0.5014008, 0.9748477),
    vec3(0.9805915, 0.1318207, 0.2371372),
    vec3(0.5980836, 0.06796348, 0.9941338),
    vec3(0.6836596, 0.9917196, 0.2319056),
    vec3(0.5276511, 0.2745509, 0.5422578),
    vec3(0.829482, 0.03758276, 0.1240466),
    vec3(0.2698198, 0.0002266169, 0.3449324)
);

float ssr_random(vec2 uv)
{
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec3 ssr_poisson_sample(int i)
{
    return POISSON3D_SAMPLES[clamp(i, 0, 127)] * 2.0 - 1.0;
}

bool ssr_inputs_available()
{
    return pc.ssr_params0.x > 0.5 &&
        pc.composite_sky_settings.x < 0.5 &&
        textureSize(sceneMap, 0).x > 1 &&
        textureSize(sceneDepthMap, 0).x > 1;
}

vec2 ssr_projected_position(vec3 pos)
{
    mat4 projection = inverse(pc.inverse_projection);
    vec4 sample_position = projection * vec4(pos, 1.0);
    sample_position.xy =
        (sample_position.xy / max(abs(sample_position.w), 0.000001)) * 0.5 +
        0.5;
    return sample_position.xy;
}

float ssr_linear_depth(vec2 tc)
{
    float depth = texture(sceneDepthMap, tc).r;
    vec3 pos = reconstruct_view_position(tc, depth);
    return -pos.z;
}

bool trace_screen_space_ray(
    vec3 position,
    vec3 reflection,
    out vec4 hit_color,
    out float hit_depth,
    float depth,
    sampler2D source)
{
    reflection += position;
    position = (soften.inverse_modelview_delta * vec4(position, 1.0)).xyz;
    reflection = (soften.inverse_modelview_delta * vec4(reflection, 1.0)).xyz;
    reflection -= position;

    depth = -position.z;
    vec3 step = max(pc.ssr_params0.z, 0.001) * reflection;
    vec3 marching_position = position + step;
    float delta = 0.0;
    float depth_from_screen = 0.0;
    vec2 screen_position = vec2(0.0);
    hit_color = vec4(0.0);

    int iteration_count = int(clamp(pc.ssr_params0.y, 1.0, 128.0));
    float distance_bias = max(pc.ssr_params0.w, 0.0001);
    float depth_reject_bias = max(pc.ssr_params1.x, 0.0);
    float adaptive_step_multiplier = max(pc.ssr_params1.z, 1.0);

    if (depth <= depth_reject_bias)
    {
        return false;
    }

    for (int i = 0; i < iteration_count; ++i)
    {
        screen_position = ssr_projected_position(marching_position);
        if (screen_position.x > 1.0 ||
            screen_position.x < 0.0 ||
            screen_position.y > 1.0 ||
            screen_position.y < 0.0)
        {
            return false;
        }

        depth_from_screen = ssr_linear_depth(screen_position);
        delta = abs(marching_position.z) - depth_from_screen;

        if (depth < depth_from_screen + 0.1 &&
            depth > depth_from_screen - 0.1)
        {
            break;
        }

        if (abs(delta) < distance_bias)
        {
            hit_color = texture(source, screen_position);
            hit_depth = depth_from_screen;
            return true;
        }

        if (delta > 0.0)
        {
            break;
        }

        float direction_sign = sign(abs(marching_position.z) - depth_from_screen);
        step *= 1.0 - max(pc.ssr_params0.z, 0.001) * max(direction_sign, 0.0);
        marching_position += step * (-direction_sign);
        step *= adaptive_step_multiplier;
    }

    for (int i = 0; i < iteration_count; ++i)
    {
        step *= 0.5;
        marching_position -= step * sign(delta);

        screen_position = ssr_projected_position(marching_position);
        if (screen_position.x > 1.0 ||
            screen_position.x < 0.0 ||
            screen_position.y > 1.0 ||
            screen_position.y < 0.0)
        {
            return false;
        }

        depth_from_screen = ssr_linear_depth(screen_position);
        delta = abs(marching_position.z) - depth_from_screen;

        if (depth < depth_from_screen + 0.1 &&
            depth > depth_from_screen - 0.1)
        {
            break;
        }

        if (abs(delta) < distance_bias &&
            depth_from_screen != (depth - distance_bias))
        {
            hit_color = texture(source, screen_position);
            hit_depth = depth_from_screen;
            return true;
        }
    }

    return false;
}

float tap_screen_space_reflection(
    vec2 tc,
    vec3 view_pos,
    vec3 norm,
    inout vec4 collected_color,
    float glossiness)
{
    collected_color = vec4(0.0);
    if (!ssr_inputs_available())
    {
        return 0.0;
    }

    int hits = 0;
    float depth = -view_pos.z;
    vec3 ray_direction = safe_normalize(reflect(view_pos, safe_normalize(norm)));
    vec2 screenpos = 1.0 - abs(tc * 2.0 - 1.0);
    float vignette = clamp((abs(screenpos.x) * abs(screenpos.y)) * 16.0, 0.0, 1.0);
    vignette *= clamp(
        (dot(safe_normalize(view_pos), norm) * 0.5 + 0.5) * 5.5 - 0.8,
        0.0,
        1.0);
    vignette *= clamp(1.0 + (view_pos.z / 128.0), 0.0, 1.0);
    vignette *= clamp(glossiness * 3.0 - 1.7, 0.0, 1.0);

    if (vignette <= 0.0)
    {
        return 0.0;
    }

    float reflection_blur = 1.0 - glossiness;
    int requested_samples = int(clamp(pc.ssr_params1.y, 1.0, 128.0));
    int total_samples = int(max(
        float(requested_samples),
        float(requested_samples) * reflection_blur * vignette));
    total_samples = clamp(total_samples, 1, 128);

    if (reflection_blur < 0.35)
    {
        for (int i = 0; i < total_samples; ++i)
        {
            vec3 first_basis =
                safe_normalize(cross(ssr_poisson_sample(i), ray_direction));
            vec3 second_basis = safe_normalize(cross(ray_direction, first_basis));
            vec2 coeffs =
                vec2(ssr_random(tc + vec2(0.0, float(i))) +
                     ssr_random(tc + vec2(float(i), 0.0)));
            vec3 randomized_direction =
                ray_direction +
                ((first_basis * coeffs.x + second_basis * coeffs.y) *
                 reflection_blur);

            vec4 hitpoint = vec4(0.0);
            float hit_depth = 0.0;
            bool hit = trace_screen_space_ray(
                view_pos,
                safe_normalize(randomized_direction),
                hitpoint,
                hit_depth,
                depth,
                sceneMap);

            if (hit)
            {
                ++hits;
                collected_color += vec4(hitpoint.rgb, 1.0);
            }
        }

        if (hits > 0)
        {
            collected_color /= float(hits);
        }
        else
        {
            collected_color = vec4(0.0);
        }
    }

    collected_color.a = (float(hits) / float(total_samples)) * vignette;
    return float(hits);
}

void sample_reflection_probes(
    inout vec3 irradiance,
    inout vec3 radiance,
    vec2 tc,
    vec3 pos,
    vec3 norm,
    float glossiness,
    vec3 fallback_ambient,
    bool classic_mode)
{
    if (!has_reflection_probe_inputs())
    {
        return;
    }

    pre_probe_sample(pos);

    if (!classic_mode)
    {
        irradiance = sample_probe_ambient(pos, norm, fallback_ambient);
    }

    float max_probe_lod = max(pc.scene_reflection.z, 0.0);
    float lod = (1.0 - glossiness) * max_probe_lod;
    radiance = sample_probe_radiance(pos, safe_normalize(reflect(pos, norm)), lod);
    if (pc.composite_sky_settings.x < 0.5 && glossiness >= 0.9)
    {
        vec4 ssr = vec4(0.0);
        tap_screen_space_reflection(tc, pos, norm, ssr, glossiness);
        radiance = mix(radiance, ssr.rgb, ssr.a);
    }
    tap_hero_probe(radiance, pos, norm, glossiness);
    radiance = clamp(radiance, vec3(0.0), vec3(10.0));
}

vec3 select_composite_light_direction()
{
    vec3 selected_light_dir =
        pc.composite_sun_direction.w > 0.5 ?
            pc.composite_sun_direction.xyz :
            pc.composite_moon_direction.xyz;
    if (dot(selected_light_dir, selected_light_dir) <= 0.0001)
    {
        selected_light_dir = pc.composite_light_direction.xyz;
    }
    if (dot(selected_light_dir, selected_light_dir) <= 0.0001)
    {
        selected_light_dir = vec3(0.32, 0.48, 0.82);
    }
    return normalize(selected_light_dir);
}

float local_light_screen_weight(vec2 texcoord, vec2 light_center, float light_radius)
{
    if (light_radius <= 0.0 ||
        light_center.x < 0.0 ||
        light_center.y < 0.0)
    {
        return 1.0;
    }

    float distance_from_light = distance(texcoord, clamp(light_center, vec2(0.0), vec2(1.0)));
    return smoothstep(light_radius, light_radius * 0.25, distance_from_light);
}

vec3 reconstruct_view_position(vec2 texcoord, float depth)
{
    vec2 ndc_xy = texcoord * 2.0 - 1.0;
    vec4 ndc = vec4(ndc_xy, depth * 2.0 - 1.0, 1.0);
    vec4 position = pc.inverse_projection * ndc;
    position.xyz /= max(abs(position.w), 0.000001);
    return position.xyz;
}

void main()
{
    vec2 tc = vary_texcoord0.xy;
    vec4 diffuse = texture(diffuseMap, tc);
    vec4 specular_or_orm = texture(specularOrOrmMap, tc);
    vec4 encoded_normal = texture(normalMap, tc);
    vec3 emissive = pc.composite_features.x > 3.5 ?
        max(texture(emissiveMap, tc).rgb, vec3(0.0)) :
        vec3(0.0);
    float scene_depth = texture(depthMap, tc).r;

    vec2 shadow_ao = texture(lightMap, tc).rg;
    float sun_shadow = max(shadow_ao.r, diffuse.a);
    float ambient_occlusion = clamp(shadow_ao.g, 0.0, 1.0);

    vec3 normal = decode_gbuffer_normal(encoded_normal);
    vec3 view_position = reconstruct_view_position(tc, scene_depth);
    vec3 light_dir = select_composite_light_direction();
    bool classic_mode = pc.composite_moon_direction.w > 0.5;
    vec3 view_dir = -safe_normalize(view_position);

    bool hdri = get_gbuffer_flag(encoded_normal.a, GBUFFER_FLAG_HAS_HDRI);
    bool skip_atmos = get_gbuffer_flag(encoded_normal.a, GBUFFER_FLAG_SKIP_ATMOS);
    bool pbr = get_gbuffer_flag(encoded_normal.a, GBUFFER_FLAG_HAS_PBR);
    vec3 base_color = max(diffuse.rgb, vec3(0.0));

    if (hdri)
    {
        frag_color = vec4(max(emissive, vec3(0.0)) * vertex_color.rgb, 0.0);
        return;
    }

    if (skip_atmos)
    {
        vec3 sky_color =
            pc.composite_features.x > 3.5 ?
                max(emissive, vec3(0.0)) :
                base_color;
        sky_color =
            srgb_to_linear(sky_color) *
            max(pc.composite_sky_settings.y, 0.0);
        frag_color = vec4(clamp_hdr_range(sky_color) * vertex_color.rgb, 0.0);
        return;
    }

    float probe_ambiance = clamp(pc.scene_reflection.x, 0.0, 1.0);
    vec3 sunlit_linear = vec3(0.0);
    vec3 ambient_color = vec3(0.0);
    vec3 atmos_additive = vec3(0.0);
    vec3 atmos_atten = vec3(1.0);
    calc_atmospheric_vars_linear(
        view_position,
        normal,
        light_dir,
        sunlit_linear,
        ambient_color,
        atmos_additive,
        atmos_atten);
    if (classic_mode)
    {
        sunlit_linear *= 1.35;
    }

    vec3 color = vec3(0.0);
    if (pbr)
    {
        vec3 orm = max(specular_or_orm.rgb, vec3(0.0));
        float perceptual_roughness = clamp(orm.g, 0.04, 1.0);
        float metallic = clamp(orm.b, 0.0, 1.0);
        float ao = clamp(orm.r, 0.0, 1.0);
        vec3 irradiance = ambient_color;
        vec3 radiance = vec3(0.0);
        sample_reflection_probes(
            irradiance,
            radiance,
            tc,
            view_position,
            normal,
            1.0 - perceptual_roughness,
            ambient_color,
            classic_mode);
        if (max(max(radiance.r, radiance.g), radiance.b) <= 0.0001)
        {
            radiance =
                sample_environment(normal, view_dir, perceptual_roughness, probe_ambiance);
        }
        adjust_irradiance(irradiance, ambient_occlusion);

        vec3 diffuse_color = vec3(0.0);
        vec3 specular_color = vec3(0.0);
        calc_diffuse_specular(base_color, metallic, diffuse_color, specular_color);
        color = pbr_base_light(
            diffuse_color,
            specular_color,
            metallic,
            view_dir,
            normal,
            perceptual_roughness,
            light_dir,
            sunlit_linear,
            sun_shadow,
            radiance,
            irradiance,
            emissive,
            ao,
            classic_mode);
    }
    else
    {
        base_color = srgb_to_linear(base_color);
        vec4 spec = specular_or_orm;
        spec.rgb = srgb_to_linear(max(spec.rgb, vec3(0.0)));
        float env_intensity = clamp(encoded_normal.b, 0.0, 1.0);

        float da = clamp(dot(normal, light_dir), 0.0, 1.0);
        vec3 irradiance = ambient_color;
        vec3 glossenv = vec3(0.0);
        vec3 legacyenv = vec3(0.0);
        sample_reflection_probes(
            irradiance,
            glossenv,
            tc,
            view_position,
            normal,
            spec.a,
            ambient_color,
            classic_mode);
        if (spec.a > 0.0 &&
            max(max(glossenv.r, glossenv.g), glossenv.b) <= 0.0001)
        {
            glossenv = sample_environment(normal, view_dir, 1.0 - spec.a, probe_ambiance);
        }
        if (env_intensity > 0.0)
        {
            vec3 legacy_irradiance = irradiance;
            sample_reflection_probes(
                legacy_irradiance,
                legacyenv,
                tc,
                view_position,
                normal,
                1.0,
                ambient_color,
                classic_mode);
            if (max(max(legacyenv.r, legacyenv.g), legacyenv.b) <= 0.0001)
            {
                legacyenv = sample_environment(normal, view_dir, 0.0, probe_ambiance);
            }
        }
        adjust_irradiance(irradiance, ambient_occlusion);

        color = irradiance;
        if (classic_mode)
        {
            da = pow(da, 1.2);
            vec3 sun_contrib = vec3(min(da, sun_shadow));
            color =
                srgb_to_linear(
                    color * 0.9 +
                    (linear_to_srgb(sun_contrib) * sunlit_linear * 0.7));
            sunlit_linear = srgb_to_linear(sunlit_linear);
        }
        else
        {
            color += min(da, sun_shadow) * sunlit_linear;
        }

        color *= base_color;

        if (spec.a > 0.0)
        {
            vec3 h;
            vec3 l;
            float nh;
            float nl;
            float nv;
            float vh;
            float light_dist;
            calc_half_vectors(
                light_dir,
                normal,
                view_dir,
                h,
                l,
                nh,
                nl,
                nv,
                vh,
                light_dist);

            if (nl > 0.0 && nh > 0.0)
            {
                float lit = min(nl * 6.0, 1.0);
                float fres = pow(1.0 - vh, 5.0) * 0.4 + 0.5;
                float gtdenom = 2.0 * nh;
                float gt = max(0.0, min(gtdenom * nv / vh, gtdenom * nl / vh));
                float scol =
                    fres *
                    texture(lightFunc, vec2(nh, spec.a)).r *
                    gt /
                    (nh * nl);
                scol *= min(nl, sun_shadow);
                color += lit * scol * sunlit_linear * spec.rgb;
            }

            apply_gloss_env(color, glossenv, spec, view_position, normal);
        }

        color = mix(color, base_color, clamp(diffuse.a, 0.0, 1.0));

        if (env_intensity > 0.0)
        {
            apply_legacy_env(color, legacyenv, spec, view_position, normal, env_intensity);
        }
    }

    float final_scale = classic_mode ? 1.1 : 1.0;
    frag_color = vec4(clamp_hdr_range(color * final_scale) * vertex_color.rgb, 0.0);
}
