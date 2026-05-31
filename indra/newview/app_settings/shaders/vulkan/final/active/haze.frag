#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D tex2;
layout(set = 0, binding = 3) uniform sampler2D tex3;
layout(set = 0, binding = 4) uniform sampler2D tex4;
layout(set = 0, binding = 5) uniform sampler2D tex5;
layout(set = 0, binding = 6) uniform sampler2D tex6;
layout(set = 0, binding = 7) uniform sampler2D tex7;
layout(set = 0, binding = 8) uniform sampler2D sceneDepthMap;
layout(set = 0, binding = 9) uniform sampler2D sceneColorMap;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 64) vec4 params;
    layout(offset = 80) vec4 material_params;
    layout(offset = 128) vec4 material_extra;
    layout(offset = 144) vec4 base_texture_transform0;
    layout(offset = 160) vec4 base_texture_transform1;
    layout(offset = 176) vec4 material_pbr;
    layout(offset = 192) vec4 material_legacy;
    layout(offset = 208) vec4 material_modes;
    layout(offset = 224) vec4 material_texture_transform2;
    layout(offset = 240) vec4 material_texture_transform3;
    layout(offset = 256) vec4 material_texture_transform4;
    layout(offset = 416) vec4 scene_ambient_direct_scale;
    layout(offset = 432) vec4 scene_direct_color;
    layout(offset = 448) vec4 scene_light_direction;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 2) flat in uint vary_texture_index;

layout(location = 0) out vec4 frag_color;

const uint MATERIAL_ATMOSPHERIC_HAZE = 8192u;
const uint MATERIAL_WATER_HAZE = 16384u;
const uint MATERIAL_WATER_EXCLUSION_MASK = 32768u;
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

vec4 diffuse_lookup(vec2 texcoord)
{
    if (pc.params.y < 0.5)
    {
        return texture(tex0, texcoord);
    }

    switch (vary_texture_index)
    {
        case 0u: return texture(tex0, texcoord);
        case 1u: return texture(tex1, texcoord);
        case 2u: return texture(tex2, texcoord);
        case 3u: return texture(tex3, texcoord);
        case 4u: return texture(tex4, texcoord);
        case 5u: return texture(tex5, texcoord);
        case 6u: return texture(tex6, texcoord);
        case 7u: return texture(tex7, texcoord);
        default: return texture(tex0, texcoord);
    }
}

vec2 screen_texcoord_from_depth()
{
    vec2 scene_size = has_material_flag(MATERIAL_HAS_SCENE_DEPTH) ?
        vec2(textureSize(sceneDepthMap, 0)) :
        vec2(textureSize(sceneColorMap, 0));
    return clamp(gl_FragCoord.xy / max(scene_size, vec2(1.0)), vec2(0.0), vec2(1.0));
}

float depth_haze_factor(vec2 screen_texcoord)
{
    if (!has_material_flag(MATERIAL_HAS_SCENE_DEPTH))
    {
        return 1.0;
    }

    float depth = texture(sceneDepthMap, screen_texcoord).r;
    if (depth >= 0.99999)
    {
        return 1.0;
    }
    return smoothstep(0.45, 0.98, depth);
}

vec4 apply_atmospheric_or_water_haze(vec4 color)
{
    vec2 screen_texcoord = screen_texcoord_from_depth();
    float haze = depth_haze_factor(screen_texcoord);
    vec3 scene_color = has_material_flag(MATERIAL_HAS_SCENE_COLOR) ?
        texture(sceneColorMap, screen_texcoord).rgb :
        color.rgb;
    vec3 fog_color = color.rgb;
    float fog_mix = 0.5;
    vec3 ambient = get_scene_ambient_color();
    vec3 direct = get_scene_direct_color();
    vec3 lit_fog_color = max(color.rgb * (ambient + direct * 0.28), vec3(0.0));

    if (has_material_flag(MATERIAL_WATER_HAZE))
    {
        float exclusion = texture(tex5, screen_texcoord).r;
        haze *= mix(1.0, exclusion, 0.65);
        vec2 ripple = vec2(
            sin((screen_texcoord.y + pc.material_params.x) * 37.0),
            cos((screen_texcoord.x + pc.material_params.y) * 29.0)) * 0.003;
        scene_color = has_material_flag(MATERIAL_HAS_SCENE_COLOR) ?
            texture(sceneColorMap, clamp(screen_texcoord + ripple, vec2(0.0), vec2(1.0))).rgb :
            scene_color;
        fog_color = mix(
            lit_fog_color,
            lit_fog_color * mix(vec3(0.72, 0.94, 1.08), direct + ambient, 0.22),
            0.35);
        fog_mix = 0.62;
    }
    else
    {
        fog_color = mix(
            lit_fog_color,
            lit_fog_color * mix(vec3(1.05, 1.08, 1.12), direct + ambient, 0.18),
            0.25);
        fog_mix = 0.42;
    }

    color.rgb = mix(scene_color, fog_color, fog_mix);
    color.a *= haze;
    return color;
}

void main()
{
    vec4 color =
        diffuse_lookup(vary_texcoord0.xy) *
        vertex_color *
        vec4(pc.material_params.rgb, pc.material_pbr.w);

    if (has_material_flag(MATERIAL_WATER_EXCLUSION_MASK))
    {
        frag_color = vec4(color.rgb, 1.0);
        return;
    }

    if (has_material_flag(MATERIAL_ATMOSPHERIC_HAZE) ||
        has_material_flag(MATERIAL_WATER_HAZE))
    {
        frag_color = apply_atmospheric_or_water_haze(color);
        return;
    }

    frag_color = color;
}
