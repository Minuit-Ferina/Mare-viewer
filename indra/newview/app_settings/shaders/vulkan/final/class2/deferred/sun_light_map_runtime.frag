#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 4) uniform sampler2D depthMap;

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

    if (encoded_normal.a < 0.5 || scene_depth >= 0.99999)
    {
        frag_color = vec4(1.0);
        return;
    }

    vec3 normal = decode_gbuffer_normal(encoded_normal);
    float ambient_occlusion = compute_depth_normal_ssao(tc, normal, scene_depth);

    // OpenGL sunLight writes directional shadow, SSAO, and two spot-shadow
    // terms. Vulkan owns SSAO here first; shadow map sampling stays neutral
    // until the Vulkan shadow render graph exists.
    frag_color = vec4(1.0, ambient_occlusion, 1.0, 1.0) * vertex_color;
}
