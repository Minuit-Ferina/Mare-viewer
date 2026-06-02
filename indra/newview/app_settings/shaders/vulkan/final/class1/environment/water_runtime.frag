#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 5) uniform sampler2D waterExclusionMap;
layout(set = 0, binding = 8) uniform sampler2D depthMap;
layout(set = 0, binding = 9) uniform sampler2D sceneColorMap;
layout(set = 0, binding = 10) uniform samplerCubeArray reflectionProbes;
layout(set = 0, binding = 11) uniform samplerCubeArray irradianceProbes;

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

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 material_params;
    layout(offset = 176) vec4 material_pbr;
    layout(offset = 272) vec4 environment0;
    layout(offset = 288) vec4 environment1;
    layout(offset = 304) vec4 environment2;
    layout(offset = 416) vec4 scene_ambient_direct_scale;
    layout(offset = 432) vec4 scene_direct_color;
    layout(offset = 448) vec4 scene_light_direction;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 3) in vec3 vary_normal;
layout(location = 5) in vec3 vary_position;

layout(location = 0) out vec4 frag_color;

const uint MATERIAL_HAS_SCENE_DEPTH = 131072u;
const uint MATERIAL_HAS_SCENE_COLOR = 262144u;

bool has_material_flag(uint flag)
{
    return (uint(pc.material_pbr.z + 0.5) & flag) != 0u;
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

vec2 screen_texcoord()
{
    vec2 scene_size = has_material_flag(MATERIAL_HAS_SCENE_DEPTH) ?
        vec2(textureSize(depthMap, 0)) :
        vec2(textureSize(sceneColorMap, 0));
    return clamp(gl_FragCoord.xy / max(scene_size, vec2(1.0)), vec2(0.0), vec2(1.0));
}

void main()
{
    vec4 water = texture(tex0, vary_texcoord0.xy) *
        vertex_color *
        vec4(pc.material_params.rgb, pc.material_pbr.w);

    vec2 screen_uv = screen_texcoord();
    float scene_depth = has_material_flag(MATERIAL_HAS_SCENE_DEPTH) ?
        texture(depthMap, screen_uv).r :
        1.0;
    float exclusion = texture(waterExclusionMap, screen_uv).r;
    float depth_fade = scene_depth >= 0.99999 ? 1.0 : smoothstep(0.2, 0.98, scene_depth);
    vec3 normal = normalize(vary_normal);
    float fresnel = pow(1.0 - clamp(abs(normal.z), 0.0, 1.0), 2.0);
    vec3 view_dir =
        safe_normalize(vec3(screen_uv * 2.0 - vec2(1.0), 1.0));
    vec3 reflection_dir = reflect(-view_dir, normal);
    vec3 probe_position = vary_position;
    float glossiness = clamp(1.0 - depth_fade * 0.35, 0.25, 1.0);
    vec3 probe_ambient =
        sample_water_probe_irradiance(
            probe_position,
            normal,
            get_scene_ambient_color());
    vec3 probe_radiance =
        sample_water_probe_radiance(probe_position, reflection_dir, glossiness);

    vec3 shallow = vec3(0.12, 0.34, 0.43);
    vec3 deep = vec3(0.03, 0.18, 0.28);
    vec3 tint = mix(shallow, deep, depth_fade);
    vec2 refraction_offset = normal.xy * mix(0.002, 0.012, depth_fade);
    vec3 scene_color = has_material_flag(MATERIAL_HAS_SCENE_COLOR) ?
        texture(sceneColorMap, clamp(screen_uv + refraction_offset, vec2(0.0), vec2(1.0))).rgb :
        water.rgb;
    water.rgb = mix(scene_color, water.rgb, 0.38);
    water.rgb = mix(water.rgb, tint, 0.42);
    vec3 light_dir = get_scene_light_direction();
    vec3 scene_light =
        max(get_scene_ambient_color(), probe_ambient) +
        get_scene_direct_color() * max(dot(normal, light_dir), 0.0);
    water.rgb *= mix(vec3(0.72), clamp(scene_light, vec3(0.0), vec3(1.35)), 0.42);
    water.rgb += get_scene_direct_color() * fresnel * 0.18;
    water.rgb += probe_radiance * fresnel * 0.22;
    water.a *= mix(0.52, 0.82, depth_fade) * mix(1.0, exclusion, 0.45);

    frag_color = water;
}
