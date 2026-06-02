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

#define MAX_REFMAP_COUNT 256
#define REF_SAMPLE_COUNT 32

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
    layout(offset = 272) vec4 composite_environment0;
    layout(offset = 288) vec4 composite_environment1;
    layout(offset = 304) vec4 composite_environment2;
    layout(offset = 352) mat4 inverse_projection;
    layout(offset = 416) vec4 scene_reflection;
} pc;

vec3 srgb_to_linear(vec3 color)
{
    bvec3 cutoff = lessThanEqual(color, vec3(0.04045));
    vec3 low = color / 12.92;
    vec3 high = pow((color + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
}

vec3 decode_gbuffer_normal(vec4 encoded)
{
    vec3 normal = normalize(encoded.xyz * 2.0 - 1.0);
    if (dot(normal, normal) <= 0.0001)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    return normal;
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

float compute_fallback_ssao(vec2 texcoord, float enabled)
{
    if (enabled < 0.5)
    {
        return 1.0;
    }

    float center_depth = texture(depthMap, texcoord).r;
    if (center_depth >= 0.99999)
    {
        return 1.0;
    }

    vec2 texel = 1.0 / max(vec2(textureSize(depthMap, 0)), vec2(1.0));
    float radius = clamp(pc.composite_ssao.x, 0.5, max(pc.composite_ssao.y, 1.0));
    float factor = clamp(pc.composite_ssao.z, 0.1, 8.0);
    float strength = clamp(pc.composite_ssao.w, 0.0, 2.0);
    vec2 sample_offset = texel * radius;

    float occlusion = 0.0;
    float sample_depth = texture(depthMap, texcoord + vec2(sample_offset.x, 0.0)).r;
    occlusion += clamp((center_depth - sample_depth) * factor * 64.0, 0.0, 1.0);
    sample_depth = texture(depthMap, texcoord - vec2(sample_offset.x, 0.0)).r;
    occlusion += clamp((center_depth - sample_depth) * factor * 64.0, 0.0, 1.0);
    sample_depth = texture(depthMap, texcoord + vec2(0.0, sample_offset.y)).r;
    occlusion += clamp((center_depth - sample_depth) * factor * 64.0, 0.0, 1.0);
    sample_depth = texture(depthMap, texcoord - vec2(0.0, sample_offset.y)).r;
    occlusion += clamp((center_depth - sample_depth) * factor * 64.0, 0.0, 1.0);

    return clamp(1.0 - occlusion * 0.25 * strength, 0.25, 1.0);
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

    if (encoded_normal.a < 0.5 || scene_depth >= 0.99999)
    {
        vec3 sky_or_color = max(diffuse.rgb, vec3(0.0));
        vec3 sky_fallback = fallback_sky_color(tc);
        if (scene_depth >= 0.99999)
        {
            sky_or_color = mix(sky_or_color, sky_fallback, 0.7);
        }
        if (max(max(sky_or_color.r, sky_or_color.g), sky_or_color.b) < 0.002)
        {
            sky_or_color = sky_fallback;
        }
        frag_color = vec4((sky_or_color + emissive) * vertex_color.rgb, 1.0);
        return;
    }

    vec2 shadow_ao = texture(lightMap, tc).rg;
    float sun_shadow = max(shadow_ao.r, diffuse.a);
    float ambient_occlusion = clamp(shadow_ao.g, 0.0, 1.0);
    if (shadow_ao.r <= 0.0001 && shadow_ao.g <= 0.0001)
    {
        sun_shadow = 1.0;
        ambient_occlusion = compute_fallback_ssao(tc, pc.composite_features.y);
    }

    vec3 normal = decode_gbuffer_normal(encoded_normal);
    vec3 view_position = reconstruct_view_position(tc, scene_depth);
    vec3 light_dir = select_composite_light_direction();
    float ndotl = max(dot(normal, light_dir), 0.0);
    bool classic_mode = pc.composite_moon_direction.w > 0.5;
    if (classic_mode)
    {
        ndotl = pow(ndotl, 1.2);
    }

    bool pbr = specular_or_orm.a > 0.5;
    vec3 base_color = max(diffuse.rgb, vec3(0.0));
    if (!pbr)
    {
        base_color = srgb_to_linear(base_color);
        specular_or_orm.rgb = srgb_to_linear(max(specular_or_orm.rgb, vec3(0.0)));
    }

    float env = clamp(diffuse.a, 0.0, 1.0);
    float ao = pbr ? clamp(specular_or_orm.r, 0.0, 1.0) : 1.0;
    float legacy_shiny = clamp(specular_or_orm.a * 2.0, 0.0, 1.0);
    float roughness = pbr ?
        clamp(specular_or_orm.g, 0.04, 1.0) :
        mix(0.72, 0.22, legacy_shiny);
    float metallic = pbr ? clamp(specular_or_orm.b, 0.0, 1.0) : 0.0;
    float combined_ao = min(ao, ambient_occlusion);

    float probe_ambiance = clamp(pc.scene_reflection.x, 0.0, 1.0);
    vec3 ambient_color = max(pc.composite_ambient.rgb, vec3(0.02));
    ambient_color = mix(
        ambient_color,
        max(ambient_color, vec3(probe_ambiance * 0.25)),
        probe_ambiance);
    vec3 direct_color = max(pc.composite_light.rgb, vec3(0.0));
    float direct_scale = max(pc.composite_light_direction.a, 0.0);
    if (classic_mode)
    {
        direct_scale *= 1.35;
    }

    vec3 view_dir = -safe_normalize(view_position);
    vec3 sampled_environment = vec3(0.0);
    sample_reflection_probes(
        ambient_color,
        sampled_environment,
        tc,
        view_position,
        normal,
        1.0 - roughness,
        ambient_color,
        classic_mode);
    if (max(max(sampled_environment.r, sampled_environment.g), sampled_environment.b) <= 0.0001)
    {
        sampled_environment =
            sample_environment(normal, view_dir, roughness, probe_ambiance);
    }
    float environment_scale = mix(1.0, 1.75, probe_ambiance);
    vec3 fallback_environment =
        base_color * env * environment_scale * mix(0.08, 0.18, 1.0 - roughness);
    vec3 environment = mix(
        fallback_environment,
        sampled_environment * env,
        clamp(probe_ambiance, 0.0, 1.0));
    vec3 ambient = base_color * ambient_color * combined_ao + environment * combined_ao;
    vec3 direct = base_color * direct_color * ndotl * direct_scale * sun_shadow * mix(1.0, 0.62, metallic);

    vec3 local_light_color = max(pc.composite_local_light.rgb, vec3(0.0));
    float local_light_strength =
        clamp(pc.composite_local_light.a, 0.0, 1.0) *
        local_light_screen_weight(
            tc,
            pc.composite_features.zw,
            pc.composite_light.a);
    vec3 local_light = base_color * local_light_color * local_light_strength *
        combined_ao * mix(0.55, 1.0, 1.0 - roughness);

    vec3 half_dir = normalize(light_dir + view_dir);
    vec3 specular_color = pbr ?
        mix(vec3(0.04), base_color, metallic) :
        max(specular_or_orm.rgb, vec3(0.0));
    float specular_power = mix(128.0, 8.0, roughness);
    float ndoth = max(dot(normal, half_dir), 0.0);
    vec3 fresnel = pbr ?
        fresnel_schlick(max(dot(half_dir, view_dir), 0.0), specular_color) :
        specular_color * mix(0.35, 1.2, legacy_shiny);
    float specular = pow(ndoth, specular_power) * (1.0 - roughness);
    vec3 direct_specular = fresnel * direct_color * specular * direct_scale * sun_shadow;
    vec3 local_specular = fresnel * local_light_color * local_light_strength *
        specular * mix(0.35, 0.75, 1.0 - roughness);

    vec3 color = ambient + direct + local_light + direct_specular + local_specular + emissive;
    frag_color = vec4(color * vertex_color.rgb, 1.0);
}
