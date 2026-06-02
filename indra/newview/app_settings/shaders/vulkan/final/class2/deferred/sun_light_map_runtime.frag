#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 4) uniform sampler2D depthMap;
layout(set = 0, binding = 9) uniform sampler2D shadowMap0;
layout(set = 0, binding = 10) uniform sampler2D shadowMap1;
layout(set = 0, binding = 11) uniform sampler2D shadowMap2;
layout(set = 0, binding = 12) uniform sampler2D shadowMap3;
layout(set = 0, binding = 13) uniform sampler2D shadowMap4;
layout(set = 0, binding = 14) uniform sampler2D shadowMap5;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 composite_ambient;
    layout(offset = 112) vec4 composite_sun_direction;
    layout(offset = 128) vec4 composite_light;
    layout(offset = 144) vec4 composite_moon_direction;
    layout(offset = 160) vec4 composite_sky_settings;
    layout(offset = 176) vec4 composite_features;
    layout(offset = 192) vec4 composite_light_direction;
    layout(offset = 208) vec4 composite_local_light;
    layout(offset = 224) vec4 composite_ssao;
    layout(offset = 416) vec4 scene_reflection;
} pc;

layout(std140, set = 1, binding = 0) uniform MareDeferredLightMapUniforms
{
    mat4 inverse_projection;
    mat4 shadow_matrix[6];
    vec4 shadow_clip;
    vec4 shadow_settings;
    vec4 shadow_resolution;
    vec4 shadow_runtime;
} u;

float sample_depth(vec2 texcoord)
{
    return texture(depthMap, clamp(texcoord, vec2(0.0), vec2(1.0))).r;
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

vec4 reconstruct_view_position(vec2 texcoord, float depth)
{
    vec2 ndc_xy = texcoord * 2.0 - 1.0;
    vec4 ndc = vec4(ndc_xy, depth * 2.0 - 1.0, 1.0);
    vec4 position = u.inverse_projection * ndc;
    position.xyz /= max(abs(position.w), 0.000001);
    return vec4(position.xyz, 1.0);
}

float compare_shadow_depth(sampler2D shadow_map, vec3 coord)
{
    if (coord.x < 0.0 || coord.x > 1.0 ||
        coord.y < 0.0 || coord.y > 1.0 ||
        coord.z < 0.0 || coord.z > 1.0)
    {
        return 1.0;
    }

    float stored_depth = texture(shadow_map, coord.xy).r;
    return coord.z <= stored_depth ? 1.0 : 0.0;
}

float pcf_directional_shadow(
    sampler2D shadow_map,
    vec4 shadow_coord,
    float bias_mul,
    vec2 pos_screen)
{
    if (shadow_coord.w <= 0.000001)
    {
        return 1.0;
    }

    vec2 shadow_res = max(u.shadow_resolution.xy, vec2(1.0));
    vec3 coord = shadow_coord.xyz / shadow_coord.w;
    coord.z += u.shadow_settings.y * bias_mul * 2.0;
    coord.x =
        floor(coord.x * shadow_res.x + fract(pos_screen.y * shadow_res.y)) /
        shadow_res.x;

    float shadow = compare_shadow_depth(shadow_map, coord) * 4.0;
    shadow += compare_shadow_depth(shadow_map, coord + vec3( 1.5 / shadow_res.x,  0.5 / shadow_res.y, 0.0));
    shadow += compare_shadow_depth(shadow_map, coord + vec3( 0.5 / shadow_res.x, -1.5 / shadow_res.y, 0.0));
    shadow += compare_shadow_depth(shadow_map, coord + vec3(-1.5 / shadow_res.x, -0.5 / shadow_res.y, 0.0));
    shadow += compare_shadow_depth(shadow_map, coord + vec3(-0.5 / shadow_res.x,  1.5 / shadow_res.y, 0.0));
    return clamp(shadow * 0.125, 0.0, 1.0);
}

float pcf_spot_shadow(
    sampler2D shadow_map,
    vec4 shadow_coord,
    float bias_scale,
    vec2 pos_screen)
{
    if (shadow_coord.w <= 0.000001)
    {
        return 1.0;
    }

    vec2 shadow_res = max(u.shadow_resolution.zw, vec2(1.0));
    vec3 coord = shadow_coord.xyz / shadow_coord.w;
    coord.z += u.shadow_settings.w * bias_scale;
    coord.x =
        floor(coord.x * shadow_res.x + fract(pos_screen.y * 0.666666666)) /
        shadow_res.x;

    vec2 offset = 1.0 / shadow_res;
    offset.y *= 1.5;

    float shadow = compare_shadow_depth(shadow_map, coord);
    shadow += compare_shadow_depth(shadow_map, coord + vec3(offset.x * 2.0, offset.y, 0.0));
    shadow += compare_shadow_depth(shadow_map, coord + vec3(offset.x, -offset.y, 0.0));
    shadow += compare_shadow_depth(shadow_map, coord + vec3(-offset.x, offset.y, 0.0));
    shadow += compare_shadow_depth(shadow_map, coord + vec3(-offset.x * 2.0, -offset.y, 0.0));
    return clamp(shadow * 0.2, 0.0, 1.0);
}

float sample_directional_shadow(vec3 position, vec3 normal, vec2 pos_screen)
{
    if (u.shadow_runtime.x < 0.5)
    {
        return 1.0;
    }

    vec3 light_dir = normalize(
        pc.composite_sun_direction.w > 0.5 ?
            pc.composite_sun_direction.xyz :
        pc.composite_moon_direction.xyz);
    float directional_lighting =
        clamp(max(0.0, dot(normal, light_dir)), 0.0, 1.0);
    vec3 shadow_position =
        position + light_dir * (1.0 - directional_lighting) * u.shadow_settings.x * 2.0;

    vec4 spos = vec4(shadow_position, 1.0);
    if (spos.z <= -u.shadow_clip.w)
    {
        return 1.0;
    }

    vec4 near_split = u.shadow_clip * -0.75;
    vec4 far_split = u.shadow_clip * -1.25;
    vec4 transition_domain = near_split - far_split;
    float shadow = 0.0;
    float weight = 0.0;

    if (spos.z < near_split.z)
    {
        float w = 1.0;
        w -= max(spos.z - far_split.z, 0.0) / max(transition_domain.z, 0.000001);
        shadow += pcf_directional_shadow(shadowMap3, u.shadow_matrix[3] * spos, 1.0, pos_screen) * w;
        weight += w;
        shadow += max((position.z + u.shadow_clip.z) / (u.shadow_clip.z - u.shadow_clip.w) * 2.0 - 1.0, 0.0);
    }
    if (spos.z < near_split.y && spos.z > far_split.z)
    {
        float w = 1.0;
        w -= max(spos.z - far_split.y, 0.0) / max(transition_domain.y, 0.000001);
        w -= max(near_split.z - spos.z, 0.0) / max(transition_domain.z, 0.000001);
        shadow += pcf_directional_shadow(shadowMap2, u.shadow_matrix[2] * spos, 1.0, pos_screen) * w;
        weight += w;
    }
    if (spos.z < near_split.x && spos.z > far_split.y)
    {
        float w = 1.0;
        w -= max(spos.z - far_split.x, 0.0) / max(transition_domain.x, 0.000001);
        w -= max(near_split.y - spos.z, 0.0) / max(transition_domain.y, 0.000001);
        shadow += pcf_directional_shadow(shadowMap1, u.shadow_matrix[1] * spos, 1.0, pos_screen) * w;
        weight += w;
    }
    if (spos.z > far_split.x)
    {
        float w = 1.0;
        w -= max(near_split.x - spos.z, 0.0) / max(transition_domain.x, 0.000001);
        shadow += pcf_directional_shadow(shadowMap0, u.shadow_matrix[0] * spos, 1.0, pos_screen) * w;
        weight += w;
    }

    return weight > 0.000001 ? clamp(shadow / weight, 0.0, 1.0) : 1.0;
}

float sample_spot_shadow(vec3 position, vec3 normal, int index, vec2 pos_screen)
{
    if (u.shadow_runtime.y < 0.5)
    {
        return 1.0;
    }

    vec4 spos = vec4(position + normal * u.shadow_settings.z, 1.0);
    if (spos.z <= -u.shadow_clip.w)
    {
        return 1.0;
    }

    vec4 near_split = u.shadow_clip * -0.75;
    vec4 far_split = u.shadow_clip * -1.25;
    vec4 transition_domain = near_split - far_split;
    float w = 1.0;
    w -= max(spos.z - far_split.z, 0.0) / max(transition_domain.z, 0.000001);

    float shadow =
        index == 0 ?
            pcf_spot_shadow(shadowMap4, u.shadow_matrix[4] * spos, 0.8, spos.xy) :
            pcf_spot_shadow(shadowMap5, u.shadow_matrix[5] * spos, 0.8, spos.xy);
    shadow = shadow * w +
        max((position.z + u.shadow_clip.z) / (u.shadow_clip.z - u.shadow_clip.w) * 2.0 - 1.0, 0.0);
    return clamp(shadow / max(w, 0.000001), 0.0, 1.0);
}

float compute_depth_normal_ssao(vec2 texcoord, vec3 normal, float center_depth)
{
    if (pc.composite_features.y < 0.5 || center_depth >= 0.99999)
    {
        return 1.0;
    }

    vec2 texel = 1.0 / max(vec2(textureSize(depthMap, 0)), vec2(1.0));
    float radius = clamp(pc.composite_ssao.x, 0.5, max(pc.composite_ssao.y, 1.0));
    float factor = clamp(pc.composite_ssao.z, 0.1, 8.0);
    float strength = clamp(pc.composite_ssao.w, 0.0, 2.0);
    vec2 sample_step = texel * radius;

    vec2 offsets[8] = vec2[](
        vec2(1.0, 0.0),
        vec2(-1.0, 0.0),
        vec2(0.0, 1.0),
        vec2(0.0, -1.0),
        vec2(0.707, 0.707),
        vec2(-0.707, 0.707),
        vec2(0.707, -0.707),
        vec2(-0.707, -0.707));

    float occlusion = 0.0;
    for (int i = 0; i < 8; ++i)
    {
        vec2 sample_coord = texcoord + offsets[i] * sample_step;
        float sample_z = sample_depth(sample_coord);
        vec4 encoded_sample_normal = texture(normalMap, clamp(sample_coord, vec2(0.0), vec2(1.0)));
        vec3 sample_normal = decode_gbuffer_normal(encoded_sample_normal);

        float depth_delta = max(center_depth - sample_z, 0.0);
        float normal_weight = clamp(1.0 - dot(normal, sample_normal), 0.0, 1.0);
        float depth_weight = clamp(depth_delta * factor * 64.0, 0.0, 1.0);
        occlusion += depth_weight * mix(0.65, 1.0, normal_weight);
    }

    return clamp(1.0 - (occlusion / 8.0) * strength, 0.2, 1.0);
}

void main()
{
    vec2 tc = vary_texcoord0.xy;
    vec4 encoded_normal = texture(normalMap, tc);
    float scene_depth = texture(depthMap, tc).r;

    if (scene_depth >= 0.99999)
    {
        frag_color = vec4(1.0);
        return;
    }

    vec3 normal = decode_gbuffer_normal(encoded_normal);
    float ambient_occlusion = compute_depth_normal_ssao(tc, normal, scene_depth);
    vec4 position = reconstruct_view_position(tc, scene_depth);

    frag_color = vec4(
        sample_directional_shadow(position.xyz, normal, tc),
        ambient_occlusion,
        sample_spot_shadow(position.xyz, normal, 0, tc),
        sample_spot_shadow(position.xyz, normal, 1, tc)) * vertex_color;
}
