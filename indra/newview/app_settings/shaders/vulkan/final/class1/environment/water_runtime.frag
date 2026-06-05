#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D bumpMap;
layout(set = 0, binding = 2) uniform sampler2D bumpMap2;
layout(set = 0, binding = 5) uniform sampler2D waterExclusionMap;
layout(set = 0, binding = 8) uniform sampler2D depthMap;
layout(set = 0, binding = 9) uniform sampler2D sceneColorMap;
layout(set = 0, binding = 10) uniform samplerCubeArray reflectionProbes;
layout(set = 0, binding = 11) uniform samplerCubeArray irradianceProbes;
layout(set = 0, binding = 12) uniform samplerCubeArray heroProbes;
layout(set = 0, binding = 13) uniform sampler2D lightMap;

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

layout(std140, set = 3, binding = 0) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 material_params;
    layout(offset = 128) vec4 clip_plane;
    layout(offset = 176) vec4 material_pbr;
    layout(offset = 208) vec4 water_time_height_fog;
    layout(offset = 240) vec4 water_settings;
    layout(offset = 256) vec4 water_normal_scale; // xyz normScale, w blend_factor
    layout(offset = 272) vec4 environment0;
    layout(offset = 288) vec4 environment1;
    layout(offset = 304) vec4 environment2;
    layout(offset = 320) vec4 water_plane;
    layout(offset = 336) vec4 water_fog_color_density;
    layout(offset = 416) vec4 scene_ambient_direct_scale;
    layout(offset = 432) vec4 scene_direct_color;
    layout(offset = 448) vec4 scene_light_direction;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 3) in vec3 vary_normal;
layout(location = 4) in vec4 vary_tangent;
layout(location = 5) in vec3 vary_position;
layout(location = 6) in vec4 refCoord;
layout(location = 7) in vec4 littleWave;
layout(location = 8) in vec4 view;

layout(location = 0) out vec4 frag_color;

const uint MATERIAL_HAS_SCENE_DEPTH = 131072u;
const uint MATERIAL_HAS_SCENE_COLOR = 262144u;
const uint MATERIAL_HAS_NORMAL_MAP = 1u;
const uint MATERIAL_HAS_LIGHT_MAP = 2097152u;
const float M_PI = 3.1415926535897932384626433832795;

bool has_material_flag(uint flag)
{
    return (uint(pc.material_pbr.z + 0.5) & flag) != 0u;
}

vec3 srgb_to_linear(vec3 color)
{
    vec3 low = color / 12.92;
    vec3 high = pow((color + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, lessThan(color, vec3(0.04045)));
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

vec3 diffuse(PBRInfo pbr_inputs)
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
    float roughness = pbr_inputs.alphaRoughness;
    float attenuation_l =
        2.0 * ndotl /
        (ndotl + sqrt(roughness * roughness + (1.0 - roughness * roughness) * (ndotl * ndotl)));
    float attenuation_v =
        2.0 * ndotv /
        (ndotv + sqrt(roughness * roughness + (1.0 - roughness * roughness) * (ndotv * ndotv)));
    return attenuation_l * attenuation_v;
}

float microfacet_distribution(PBRInfo pbr_inputs)
{
    float roughness_sq = pbr_inputs.alphaRoughness * pbr_inputs.alphaRoughness;
    float f = (pbr_inputs.NdotH * roughness_sq - pbr_inputs.NdotH) * pbr_inputs.NdotH + 1.0;
    return roughness_sq / (M_PI * f * f);
}

void calc_diffuse_specular(
    vec3 base_color,
    float metallic,
    inout vec3 diffuse_color,
    inout vec3 specular_color)
{
    vec3 f0 = vec3(0.04);
    diffuse_color = base_color * (vec3(1.0) - f0);
    diffuse_color *= 1.0 - metallic;
    specular_color = mix(f0, base_color, metallic);
}

void pbr_punctual(
    vec3 diffuse_color,
    vec3 specular_color,
    float perceptual_roughness,
    float metallic,
    vec3 normal,
    vec3 view_vector,
    vec3 light_vector,
    out float nl,
    out vec3 diff,
    out vec3 spec)
{
    perceptual_roughness = max(perceptual_roughness, 8.0 / 255.0);
    float alpha_roughness = perceptual_roughness * perceptual_roughness;
    float reflectance = max(max(specular_color.r, specular_color.g), specular_color.b);
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 specular_environment_r0 = specular_color.rgb;
    vec3 specular_environment_r90 = vec3(1.0) * reflectance90;

    vec3 half_vector = normalize(light_vector + view_vector);
    float ndotl = clamp(dot(normal, light_vector), 0.001, 1.0);
    float ndotv = clamp(abs(dot(normal, view_vector)), 0.001, 1.0);
    float ndoth = clamp(dot(normal, half_vector), 0.0, 1.0);
    float ldoth = clamp(dot(light_vector, half_vector), 0.0, 1.0);
    float vdoth = clamp(dot(view_vector, half_vector), 0.0, 1.0);

    PBRInfo pbr_inputs = PBRInfo(
        ndotl,
        ndotv,
        ndoth,
        ldoth,
        vdoth,
        perceptual_roughness,
        metallic,
        specular_environment_r0,
        specular_environment_r90,
        alpha_roughness,
        diffuse_color,
        specular_color);

    vec3 fresnel = specular_reflection(pbr_inputs);
    float geometric = geometric_occlusion(pbr_inputs);
    float distribution = microfacet_distribution(pbr_inputs);
    diff = (1.0 - fresnel) * diffuse(pbr_inputs);
    spec = fresnel * geometric * distribution / (4.0 * ndotl * ndotv);
    nl = ndotl;
}

vec3 get_scene_ambient_color()
{
    if (pc.scene_direct_color.a < 0.5)
    {
        return vec3(0.36);
    }
    return max(pc.scene_ambient_direct_scale.rgb, vec3(0.0));
}

vec3 get_scene_direct_color()
{
    if (pc.scene_direct_color.a < 0.5)
    {
        return vec3(1.0);
    }
    return max(pc.scene_direct_color.rgb, vec3(0.0)) *
        max(pc.scene_ambient_direct_scale.a, 0.0);
}

vec3 get_scene_light_direction()
{
    vec3 light_dir = pc.scene_light_direction.xyz;
    if (pc.scene_light_direction.w < 0.5 ||
        dot(light_dir, light_dir) <= 0.0001)
    {
        light_dir = vec3(0.35, 0.45, 0.82);
    }
    return normalize(light_dir);
}

vec3 safe_normalize(vec3 value)
{
    float len2 = dot(value, value);
    if (len2 <= 0.000001)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    return value * inversesqrt(len2);
}

bool has_water_probe_inputs()
{
    return probes.refmapCount > 0 &&
        textureSize(reflectionProbes, 0).z > 0 &&
        textureSize(irradianceProbes, 0).z > 0;
}

int waterProbeIndex[REF_SAMPLE_COUNT];
int waterProbeInfluences = 0;

mat3 water_env_mat()
{
    return mat3(pc.environment0.xyz, pc.environment1.xyz, pc.environment2.xyz);
}

bool should_sample_water_probe(int i, vec3 pos)
{
    if (i < 0 || i >= probes.refmapCount || i >= MAX_REFMAP_COUNT)
    {
        return false;
    }

    if (probes.refIndex[i].w < 0)
    {
        vec4 local_pos = probes.refBox[i] * vec4(pos, 1.0);
        return abs(local_pos.x) <= 1.0 &&
            abs(local_pos.y) <= 1.0 &&
            abs(local_pos.z) <= 1.0;
    }

    if (probes.refIndex[i].w == 0)
    {
        return false;
    }

    vec3 delta = pos - probes.refSphere[i].xyz;
    float radius = probes.refSphere[i].w;
    return dot(delta, delta) <= radius * radius;
}

void append_water_probe_index(int index)
{
    if (waterProbeInfluences >= REF_SAMPLE_COUNT)
    {
        return;
    }
    for (int i = 0; i < waterProbeInfluences; ++i)
    {
        if (waterProbeIndex[i] == index)
        {
            return;
        }
    }
    waterProbeIndex[waterProbeInfluences] = index;
    ++waterProbeInfluences;
}

int water_probe_start_index(vec3 pos)
{
    int bucket = clamp(int(floor(-pos.z)), 0, 255);
    return clamp(probes.refBucket[bucket].x, 1, probes.refmapCount + 1);
}

void pre_sample_water_probes(vec3 pos)
{
    waterProbeInfluences = 0;
    if (!has_water_probe_inputs())
    {
        return;
    }

    int start = water_probe_start_index(pos);
    for (int i = start; i < probes.refmapCount && waterProbeInfluences < REF_SAMPLE_COUNT; ++i)
    {
        if (!should_sample_water_probe(i, pos))
        {
            continue;
        }

        append_water_probe_index(i);

        int neighbor_idx = probes.refIndex[i].y;
        if (neighbor_idx != -1)
        {
            int neighbor_count = probes.refIndex[i].z;
            int count = 0;
            while (count < neighbor_count &&
                   neighbor_idx >= 0 &&
                   neighbor_idx < 1024 &&
                   waterProbeInfluences < REF_SAMPLE_COUNT)
            {
                ivec4 neighbors = probes.refNeighbor[neighbor_idx];
                for (int component = 0;
                     component < 4 &&
                        count < neighbor_count &&
                        waterProbeInfluences < REF_SAMPLE_COUNT;
                     ++component)
                {
                    int idx = component == 0 ? neighbors.x :
                        (component == 1 ? neighbors.y :
                        (component == 2 ? neighbors.z : neighbors.w));
                    if (should_sample_water_probe(idx, pos))
                    {
                        append_water_probe_index(idx);
                    }
                    ++count;
                }
                ++neighbor_idx;
            }
        }
        break;
    }

    append_water_probe_index(0);
}

int water_probe_layer(samplerCubeArray probe_texture, int i)
{
    int layer_count = textureSize(probe_texture, 0).z;
    if (layer_count <= 0)
    {
        return -1;
    }
    return clamp(probes.refIndex[i].x, 0, layer_count - 1);
}

vec3 water_sphere_intersect(vec3 origin, vec3 dir, vec3 center, float radius2)
{
    vec3 offset = center - origin;
    float projection = dot(offset, dir);
    float distance2 = max(dot(offset, offset) - projection * projection, 0.0);
    float half_chord = sqrt(max(radius2 - distance2, 0.0));
    return origin + dir * (projection + half_chord);
}

vec3 water_box_intersect(vec3 origin, vec3 dir, mat4 clip_to_local, out float distance_to_box)
{
    vec3 ray_local = mat3(clip_to_local) * dir;
    vec3 position_local = (clip_to_local * vec4(origin, 1.0)).xyz;
    distance_to_box =
        1.0 - max(max(abs(position_local.x), abs(position_local.y)), abs(position_local.z));

    vec3 safe_ray = vec3(
        abs(ray_local.x) > 0.000001 ? ray_local.x : (ray_local.x < 0.0 ? -0.000001 : 0.000001),
        abs(ray_local.y) > 0.000001 ? ray_local.y : (ray_local.y < 0.0 ? -0.000001 : 0.000001),
        abs(ray_local.z) > 0.000001 ? ray_local.z : (ray_local.z < 0.0 ? -0.000001 : 0.000001));
    vec3 first_plane = (vec3(1.0) - position_local) / safe_ray;
    vec3 second_plane = (vec3(-1.0) - position_local) / safe_ray;
    vec3 furthest_plane = max(first_plane, second_plane);
    float distance =
        min(furthest_plane.x, min(furthest_plane.y, furthest_plane.z));
    return origin + dir * distance;
}

float water_sphere_weight(
    vec3 pos,
    vec3 dir,
    vec3 origin,
    float radius,
    vec4 params,
    out float distance_weight)
{
    float inner_radius = radius * 0.5;
    float distance_to_probe = max(length(pos - origin), 0.001);
    float attenuation =
        1.0 - max(distance_to_probe - inner_radius, 0.0) /
            max(radius - inner_radius, 0.001);
    float weight = params.z / distance_to_probe;
    distance_weight = weight * attenuation * max(radius, 1.0) * 4.0;
    return weight * attenuation;
}

vec3 tap_water_reflection_map(
    vec3 pos,
    vec3 dir,
    out float weight,
    out float distance_weight,
    float lod,
    int i)
{
    weight = 0.0;
    distance_weight = 0.0;

    int layer = water_probe_layer(reflectionProbes, i);
    if (layer < 0)
    {
        return vec3(0.0);
    }

    vec3 sample_pos;
    if (probes.refIndex[i].w < 0)
    {
        float distance_to_box = 0.0;
        sample_pos = water_box_intersect(pos, dir, probes.refBox[i], distance_to_box);
        weight = max(distance_to_box, 0.001);
        distance_weight = weight;
    }
    else
    {
        float radius = probes.refSphere[i].w;
        float radius2 =
            probes.refIndex[i].w < 1 ?
                4096.0 * 4096.0 :
                radius * radius;
        sample_pos = water_sphere_intersect(pos, dir, probes.refSphere[i].xyz, radius2);
        weight =
            water_sphere_weight(
                pos,
                dir,
                probes.refSphere[i].xyz,
                radius,
                probes.refParams[i],
                distance_weight);
    }

    vec3 sample_dir =
        water_env_mat() * (sample_pos - probes.refSphere[i].xyz);
    return textureLod(
        reflectionProbes,
        vec4(safe_normalize(sample_dir), layer),
        lod).rgb *
        max(probes.refParams[i].y, 0.0);
}

vec3 tap_water_irradiance_map(
    vec3 pos,
    vec3 dir,
    out float weight,
    out float distance_weight,
    int i,
    vec3 fallback_ambient)
{
    weight = 0.0;
    distance_weight = 0.0;

    int layer = water_probe_layer(irradianceProbes, i);
    if (layer < 0)
    {
        return fallback_ambient;
    }

    vec3 sample_pos;
    if (probes.refIndex[i].w < 0)
    {
        float distance_to_box = 0.0;
        sample_pos = water_box_intersect(pos, dir, probes.refBox[i], distance_to_box);
        weight = max(distance_to_box, 0.001);
        distance_weight = weight;
    }
    else
    {
        float radius = probes.refSphere[i].w;
        float radius2 =
            probes.refIndex[i].w < 1 ?
                4096.0 * 4096.0 :
                radius * radius;
        sample_pos = water_sphere_intersect(pos, dir, probes.refSphere[i].xyz, radius2);
        weight =
            water_sphere_weight(
                pos,
                dir,
                probes.refSphere[i].xyz,
                radius,
                probes.refParams[i],
                distance_weight);
    }

    vec3 sample_dir =
        water_env_mat() * (sample_pos - probes.refSphere[i].xyz);
    vec3 color =
        textureLod(
            irradianceProbes,
            vec4(safe_normalize(sample_dir), layer),
            0.0).rgb *
        max(probes.refParams[i].x, 0.0);
    return mix(fallback_ambient, color, clamp(probes.refParams[i].x, 0.0, 1.0));
}

vec3 sample_water_probe_irradiance(vec3 pos, vec3 normal, vec3 fallback_ambient)
{
    if (!has_water_probe_inputs())
    {
        return fallback_ambient;
    }

    pre_sample_water_probes(pos);
    if (waterProbeInfluences <= 0)
    {
        return fallback_ambient;
    }

    float weight_auto = 0.0;
    float weight_manual = 0.0;
    float distance_weight_manual = 0.0;
    vec3 color_auto = vec3(0.0);
    vec3 color_manual = vec3(0.0);

    for (int idx = 0; idx < waterProbeInfluences; ++idx)
    {
        int i = waterProbeIndex[idx];
        int probe_type = clamp(abs(probes.refIndex[i].w), 0, 1);
        float weight = 0.0;
        float distance_weight = 0.0;
        vec3 probe_color =
            tap_water_irradiance_map(
                pos,
                normal,
                weight,
                distance_weight,
                i,
                fallback_ambient);
        if (probe_type == 0)
        {
            color_auto += probe_color * weight;
            weight_auto += weight;
        }
        else
        {
            color_manual += probe_color * weight;
            weight_manual += weight;
            distance_weight_manual += distance_weight;
        }
    }

    if (weight_auto > 0.0)
    {
        color_auto /= weight_auto;
    }
    if (weight_manual > 0.0)
    {
        color_manual /= weight_manual;
        return mix(color_auto, color_manual, clamp(distance_weight_manual, 0.0, 1.0));
    }
    return weight_auto > 0.0 ? color_auto : fallback_ambient;
}

vec3 sample_water_probe_radiance(vec3 pos, vec3 direction, float glossiness)
{
    if (!has_water_probe_inputs())
    {
        return vec3(0.0);
    }

    pre_sample_water_probes(pos);
    if (waterProbeInfluences <= 0)
    {
        return vec3(0.0);
    }

    float mip_count = max(float(textureQueryLevels(reflectionProbes) - 1), 0.0);
    float lod = (1.0 - glossiness) * mip_count;

    float weight_auto = 0.0;
    float weight_manual = 0.0;
    float distance_weight_manual = 0.0;
    vec3 color_auto = vec3(0.0);
    vec3 color_manual = vec3(0.0);

    for (int idx = 0; idx < waterProbeInfluences; ++idx)
    {
        int i = waterProbeIndex[idx];
        int probe_type = clamp(abs(probes.refIndex[i].w), 0, 1);
        float weight = 0.0;
        float distance_weight = 0.0;
        vec3 probe_color =
            tap_water_reflection_map(
                pos,
                direction,
                weight,
                distance_weight,
                lod,
                i);
        if (probe_type == 0)
        {
            color_auto += probe_color * weight;
            weight_auto += weight;
        }
        else
        {
            color_manual += probe_color * weight;
            weight_manual += weight;
            distance_weight_manual += distance_weight;
        }
    }

    if (weight_auto > 0.0)
    {
        color_auto /= weight_auto;
    }
    if (weight_manual > 0.0)
    {
        color_manual /= weight_manual;
        return mix(color_auto, color_manual, clamp(distance_weight_manual, 0.0, 1.0));
    }
    return weight_auto > 0.0 ? color_auto : vec3(0.0);
}

vec3 get_water_probe_normal(vec3 normal)
{
    vec3 scale = max(abs(pc.water_normal_scale.xyz), vec3(0.001));
    return safe_normalize(normal * scale);
}

vec3 sample_water_bump(sampler2D bump_sampler, vec2 uv)
{
    return texture(bump_sampler, uv).xyz * 2.0 - 1.0;
}

vec3 transform_water_normal(vec3 normal_tangent_space, vec3 fallback_normal)
{
    vec3 n = safe_normalize(fallback_normal);
    vec3 t = safe_normalize(vary_tangent.xyz);
    vec3 b = safe_normalize(cross(n, t)) * (vary_tangent.w < 0.0 ? -1.0 : 1.0);
    return safe_normalize(
        normal_tangent_space.x * t +
            normal_tangent_space.y * b +
            normal_tangent_space.z * n);
}

vec3 get_water_wave_normal(
    vec3 fallback_normal,
    out vec3 wave1,
    out vec3 wave2,
    out vec3 wave3,
    out vec3 wavef)
{
    wave1 = vec3(0.0, 0.0, 1.0);
    wave2 = vec3(0.0, 0.0, 1.0);
    wave3 = vec3(0.0, 0.0, 1.0);
    wavef = vec3(0.0, 0.0, 1.0);

    if (!has_material_flag(MATERIAL_HAS_NORMAL_MAP))
    {
        return fallback_normal;
    }

    vec2 big_wave = vec2(refCoord.w, view.w);
    vec2 little_wave0 = littleWave.xy;
    vec2 little_wave1 = littleWave.zw;

    vec3 wave1_a = sample_water_bump(bumpMap, big_wave);
    vec3 wave2_a = sample_water_bump(bumpMap, little_wave0);
    vec3 wave3_a = sample_water_bump(bumpMap, little_wave1);

    vec3 wave1_b = sample_water_bump(bumpMap2, big_wave);
    vec3 wave2_b = sample_water_bump(bumpMap2, little_wave0);
    vec3 wave3_b = sample_water_bump(bumpMap2, little_wave1);

    float blend_factor = clamp(pc.water_normal_scale.w, 0.0, 1.0);
    wave1 = mix(wave1_a, wave1_b, blend_factor);
    wave2 = mix(wave2_a, wave2_b, blend_factor);
    wave3 = mix(wave3_a, wave3_b, blend_factor);
    wavef = safe_normalize((wave1 + wave2 * 0.4 + wave3 * 0.6) * 0.5);
    return transform_water_normal(wavef, fallback_normal);
}

float get_water_glossiness()
{
    return clamp(1.0 - max(pc.water_settings.z, 0.0), 0.0, 1.0);
}

void calculate_water_fresnel_factors(
    out vec3 df3,
    out vec2 df2,
    vec3 view_vec,
    vec3 wave1,
    vec3 wave2,
    vec3 wave3,
    vec3 wavef)
{
    df3 = max(
        vec3(0.0),
        vec3(
            dot(view_vec, wave1),
            dot(view_vec, (wave2 + wave3) * 0.5),
            dot(view_vec, wave3)) *
            pc.water_settings.x +
            pc.water_settings.y);
    df3 *= df3;
    df2 = max(
        vec2(0.0),
        vec2(
            df3.x + df3.y + df3.z,
            dot(view_vec, wavef) * pc.water_settings.x + pc.water_settings.y));
}

vec2 get_water_refraction_offset(vec3 normal, float depth_fade, float distance_to_eye)
{
    float dmod = sqrt(max(distance_to_eye, 1.0));
    float source_offset =
        pc.water_settings.w / max(dmod, 1.0) * 2.0;
    float fallback_offset = mix(0.002, 0.012, depth_fade);
    return normal.xy * (pc.water_settings.w > 0.0 ? source_offset : fallback_offset);
}

vec4 get_water_fog_view_no_clip(vec3 pos)
{
    float water_fog_density = max(pc.water_time_height_fog.z, 0.0);
    if (water_fog_density <= 0.0)
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    vec3 view_dir = safe_normalize(pos);
    float es = -(dot(view_dir, pc.water_plane.xyz));
    if (abs(es) <= 0.00001)
    {
        es = es < 0.0 ? -0.00001 : 0.00001;
    }

    float eye_depth = max(-pc.water_plane.w, 0.0);
    vec3 intersection =
        pc.water_plane.w > 0.0 ?
            view_dir * pc.water_plane.w / es :
            vec3(0.0);
    float depth = length(pos - intersection);
    float water_thickness = max(depth, 0.1);

    float kd = water_fog_density;
    float ks = max(pc.water_time_height_fog.w, 0.0);
    vec3 fog_color = pc.water_fog_color_density.rgb;
    const float fog_base = 0.98;

    float t1 = -kd * pow(fog_base, ks * eye_depth);
    float t2 = kd + ks * es;
    if (abs(t2) <= 0.00001)
    {
        t2 = t2 < 0.0 ? -0.00001 : 0.00001;
    }
    float t3 = pow(fog_base, t2 * water_thickness) - 1.0;
    float light_term = pow(clamp(t1 / t2 * t3, 0.0, 1.0), 1.0 / 1.7);
    float density_term = pow(fog_base, water_thickness * kd);

    return vec4(srgb_to_linear(fog_color) * light_term, density_term);
}

vec4 apply_water_fog_view_linear(vec3 pos, vec4 color)
{
    if (dot(pos, pc.water_plane.xyz) + pc.water_plane.w > 0.0)
    {
        return color;
    }

    vec4 fogged = get_water_fog_view_no_clip(pos);
    color.rgb = color.rgb * fogged.a + fogged.rgb;
    return color;
}

void tap_water_hero_probe(
    inout vec3 radiance,
    vec3 pos,
    vec3 normal,
    vec3 reflected,
    float glossiness)
{
    if (probes.heroProbeCount <= 0 || textureSize(heroProbes, 0).z <= 0)
    {
        return;
    }

    float weight = 0.0;
    float distance_weight = 0.0;
    const float falloff_mult = 10.0;
    float clip_dist = 1.0;
    if (dot(pc.clip_plane.xyz, pc.clip_plane.xyz) > 0.000001)
    {
        clip_dist = dot(pos, pc.clip_plane.xyz) + pc.clip_plane.w;
        clip_dist = clip_dist * 0.95 + 0.05;
    }
    if (probes.heroShape < 1)
    {
        float distance_to_box = 0.0;
        water_box_intersect(pos, normal, probes.heroBox, distance_to_box);
        weight = max(distance_to_box, 0.0);
    }
    else
    {
        weight =
            water_sphere_weight(
                pos,
                reflected,
                probes.heroSphere.xyz,
                probes.heroSphere.w,
                vec4(1.0),
                distance_weight);
    }

    clip_dist = clamp(clip_dist * falloff_mult, 0.0, 1.0);
    weight = clamp(weight * falloff_mult * clip_dist, 0.0, 1.0);
    weight = mix(0.0, weight, clamp(glossiness - 0.75, 0.0, 1.0) * 4.0);

    float hero_lod = (1.0 - glossiness) * max(float(probes.heroMipCount), 0.0);
    vec3 hero_dir = water_env_mat() * reflected;
    vec3 hero_color =
        textureLod(
            heroProbes,
            vec4(safe_normalize(hero_dir), 0),
            hero_lod).rgb;
    radiance = mix(radiance, hero_color, weight);
}

vec2 screen_texcoord()
{
    vec2 scene_size = has_material_flag(MATERIAL_HAS_SCENE_DEPTH) ?
        vec2(textureSize(depthMap, 0)) :
        vec2(textureSize(sceneColorMap, 0));
    return clamp(gl_FragCoord.xy / max(scene_size, vec2(1.0)), vec2(0.0), vec2(1.0));
}

vec2 water_refraction_texcoord()
{
    if (abs(refCoord.z) <= 0.000001)
    {
        return screen_texcoord();
    }
    return clamp((refCoord.xy / refCoord.z) * 0.5 + 0.5, vec2(0.0), vec2(0.999));
}

void main()
{
    vec4 water = texture(tex0, vary_texcoord0.xy) *
        vertex_color *
        vec4(pc.material_params.rgb, pc.material_pbr.w);

    vec2 screen_uv = water_refraction_texcoord();
    float water_shadow = has_material_flag(MATERIAL_HAS_LIGHT_MAP) ?
        clamp(texture(lightMap, screen_uv).r, 0.0, 1.0) :
        1.0;
    float scene_depth = has_material_flag(MATERIAL_HAS_SCENE_DEPTH) ?
        texture(depthMap, screen_uv).r :
        1.0;
    float exclusion = texture(waterExclusionMap, screen_uv).r;
    float depth_fade = scene_depth >= 0.99999 ? 1.0 : smoothstep(0.2, 0.98, scene_depth);
    vec3 normal = safe_normalize(vary_normal);
    vec3 wave1 = vec3(0.0);
    vec3 wave2 = vec3(0.0);
    vec3 wave3 = vec3(0.0);
    vec3 wavef = vec3(0.0);
    vec3 wave_normal = get_water_wave_normal(normal, wave1, wave2, wave3, wavef);
    vec3 probe_position = vary_position;
    vec3 probe_normal = get_water_probe_normal(wave_normal);
    vec3 view_vector = safe_normalize(view.xyz);
    vec3 fresnel_df3 = vec3(0.0);
    vec2 fresnel_df2 = vec2(0.0);
    calculate_water_fresnel_factors(
        fresnel_df3,
        fresnel_df2,
        view_vector,
        wave1,
        wave2,
        wave3,
        wavef);
    float reflection_mix = min(1.0, fresnel_df2.x);
    float radiance_scale = max(fresnel_df2.y, 0.0);
    float distance_to_eye = length(view.xyz);
    vec3 view_dir =
        safe_normalize(vec3(screen_uv * 2.0 - vec2(1.0), 1.0));
    vec3 reflection_dir = reflect(-view_dir, probe_normal);
    float glossiness = get_water_glossiness();
    vec3 probe_radiance =
        sample_water_probe_radiance(probe_position, reflection_dir, glossiness);
    tap_water_hero_probe(
        probe_radiance,
        probe_position,
        probe_normal,
        reflection_dir,
        glossiness);
    probe_radiance *= radiance_scale;

    vec2 refraction_offset =
        get_water_refraction_offset(probe_normal, depth_fade, distance_to_eye);
    vec3 scene_color = has_material_flag(MATERIAL_HAS_SCENE_COLOR) ?
        texture(sceneColorMap, clamp(screen_uv + refraction_offset, vec2(0.0), vec2(1.0))).rgb :
        water.rgb;

    float metallic = 1.0;
    float perceptual_roughness = max(pc.water_settings.z, 0.0);
    vec3 diffuse_color = vec3(0.0);
    vec3 specular_color = vec3(0.0);
    calc_diffuse_specular(
        srgb_to_linear(max(get_scene_direct_color(), vec3(0.0))),
        metallic,
        diffuse_color,
        specular_color);

    vec3 light_dir = get_scene_light_direction();
    vec3 view_surface_to_camera = -safe_normalize(probe_position);
    vec3 up = normal;
    float vdu = clamp(-dot(safe_normalize(probe_position), up) * 2.0, 0.0, 1.0);
    vec3 punctual_normal =
        safe_normalize(wave_normal + up * max(distance_to_eye, 32.0) / 32.0 * (1.0 - vdu));
    float nl = 0.0;
    vec3 diff_punctual = vec3(0.0);
    vec3 spec_punctual = vec3(0.0);
    pbr_punctual(
        diffuse_color,
        specular_color,
        perceptual_roughness,
        metallic,
        punctual_normal,
        view_surface_to_camera,
        normalize(light_dir),
        nl,
        diff_punctual,
        spec_punctual);
    vec3 punctual =
        clamp(nl * (diff_punctual + spec_punctual), vec3(0.0), vec3(10.0)) *
        max(get_scene_direct_color(), vec3(0.0)) *
        water_shadow;
    vec3 source_water_color = mix(scene_color, probe_radiance, reflection_mix) + punctual;
    float fade = min(1.0, depth_fade * exclusion * 60.0);
    water.rgb = mix(scene_color, source_water_color, fade);
    water = apply_water_fog_view_linear(probe_position, water);
    water.a *= mix(0.52, 0.82, depth_fade) * mix(1.0, exclusion, 0.45);

    frag_color = water;
}
