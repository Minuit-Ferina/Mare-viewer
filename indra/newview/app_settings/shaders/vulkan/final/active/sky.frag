#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 material_params;
    layout(offset = 176) vec4 material_pbr;
    layout(offset = 416) vec4 scene_ambient_direct_scale;
    layout(offset = 432) vec4 scene_direct_color;
    layout(offset = 448) vec4 scene_light_direction;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 5) in vec3 vary_position;

layout(location = 0) out vec4 frag_color;

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

void main()
{
    vec3 base_color = max(pc.material_params.rgb * vertex_color.rgb, vec3(0.0));
    vec3 sky_direction = normalize(vary_position);
    if (dot(sky_direction, sky_direction) <= 0.0001)
    {
        sky_direction = vec3(0.0, 1.0, 0.0);
    }

    float altitude = clamp(sky_direction.y, -0.25, 1.0);
    float horizon = smoothstep(-0.08, 0.22, altitude);
    float zenith = smoothstep(0.18, 0.92, altitude);
    float rim = 1.0 - smoothstep(-0.02, 0.26, abs(altitude));
    float light_alignment = max(dot(sky_direction, get_scene_light_direction()), 0.0);

    vec3 ambient = get_scene_ambient_color();
    vec3 direct = get_scene_direct_color();
    vec3 horizon_color = base_color * (ambient * 1.10 + direct * 0.22) * vec3(1.28, 1.16, 0.98);
    vec3 mid_color = base_color * (ambient * 0.92 + direct * 0.14) * vec3(0.92, 1.02, 1.14);
    vec3 zenith_color = base_color * (ambient * 0.72 + direct * 0.20) * vec3(0.62, 0.78, 1.28);
    vec3 rgb = mix(horizon_color, mid_color, horizon);
    rgb = mix(rgb, zenith_color, zenith);
    rgb += base_color * rim * 0.08;
    rgb += base_color * direct * pow(light_alignment, 12.0) * 0.18;

    frag_color = vec4(rgb, pc.material_pbr.w * vertex_color.a);
}
