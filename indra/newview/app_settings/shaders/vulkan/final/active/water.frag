#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 5) uniform sampler2D waterExclusionMap;
layout(set = 0, binding = 8) uniform sampler2D depthMap;
layout(set = 0, binding = 9) uniform sampler2D sceneColorMap;

layout(std140, set = 3, binding = 0) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 material_params;
    layout(offset = 176) vec4 material_pbr;
    layout(offset = 416) vec4 scene_ambient_direct_scale;
    layout(offset = 432) vec4 scene_direct_color;
    layout(offset = 448) vec4 scene_light_direction;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 3) in vec3 vary_normal;

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
        get_scene_ambient_color() +
        get_scene_direct_color() * max(dot(normal, light_dir), 0.0);
    water.rgb *= mix(vec3(0.72), clamp(scene_light, vec3(0.0), vec3(1.35)), 0.42);
    water.rgb += get_scene_direct_color() * fresnel * 0.18;
    water.a *= mix(0.52, 0.82, depth_fade) * mix(1.0, exclusion, 0.45);

    frag_color = water;
}
