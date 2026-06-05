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

layout(std140, set = 3, binding = 0) uniform MareWorldPushConstants
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
layout(location = 3) in vec3 vary_normal;
layout(location = 4) in vec4 vary_tangent;
layout(location = 6) in vec2 vary_material_texcoord0;
layout(location = 7) in vec2 vary_material_texcoord1;
layout(location = 8) in vec2 vary_material_texcoord2;

layout(location = 0) out vec4 frag_color;

const uint MATERIAL_HAS_NORMAL_MAP = 1u;
const uint MATERIAL_HAS_ORM_MAP = 2u;
const uint MATERIAL_FULLBRIGHT = 4u;
const uint MATERIAL_GLOW = 8u;
const uint MATERIAL_HAS_SPECULAR_MAP = 32u;
const uint MATERIAL_DOUBLE_SIDED = 256u;
const uint MATERIAL_LEGACY_BUMP = 512u;
const uint MATERIAL_LEGACY_SHINY = 1024u;
const uint MATERIAL_GLTF_PBR = 2048u;
const uint MATERIAL_HAS_SCENE_DEPTH = 131072u;
const uint MATERIAL_SCENE_DEPTH_FLIP_Y = 524288u;
const uint MATERIAL_SCENE_DEPTH_REVERSED = 1048576u;

bool has_material_flag(uint flag)
{
    return (uint(pc.material_pbr.z + 0.5) & flag) != 0u;
}

vec3 get_scene_ambient_color(float fallback)
{
    if (pc.scene_direct_color.a < 0.5)
    {
        return vec3(fallback);
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

vec2 apply_slot_texture_transform(vec2 texcoord, vec2 scale, float rotation, vec2 offset)
{
    texcoord.y = 1.0 - texcoord.y;
    mat3 scale_mat = mat3(scale.x, 0.0, 0.0, 0.0, scale.y, 0.0, 0.0, 0.0, 1.0);
    mat3 offset_mat = mat3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, offset.x, offset.y, 1.0);
    mat3 rotation_mat = mat3(
        cos(rotation), -sin(rotation), 0.0,
        sin(rotation),  cos(rotation), 0.0,
        0.0,           0.0,          1.0);
    texcoord = (offset_mat * rotation_mat * scale_mat * vec3(texcoord, 1.0)).xy;
    texcoord.y = 1.0 - texcoord.y;
    return texcoord;
}

vec2 normal_texture_texcoord(vec2 texcoord)
{
    return apply_slot_texture_transform(
        texcoord,
        pc.base_texture_transform1.yz,
        pc.base_texture_transform1.w,
        pc.material_texture_transform2.xy);
}

vec2 orm_texture_texcoord(vec2 texcoord)
{
    return apply_slot_texture_transform(
        texcoord,
        pc.material_texture_transform2.zw,
        pc.material_texture_transform3.x,
        pc.material_texture_transform3.yz);
}

vec2 emissive_texture_texcoord(vec2 texcoord)
{
    return apply_slot_texture_transform(
        texcoord,
        vec2(pc.material_texture_transform3.w, pc.material_texture_transform4.x),
        pc.material_texture_transform4.y,
        pc.material_texture_transform4.zw);
}

vec2 normal_sample_texcoord(vec2 texcoord)
{
    return has_material_flag(MATERIAL_GLTF_PBR) ?
        normal_texture_texcoord(texcoord) :
        vary_material_texcoord1.xy;
}

vec2 legacy_specular_sample_texcoord()
{
    return vary_material_texcoord2.xy;
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

vec4 apply_alpha_policy(vec4 color)
{
    if (has_material_flag(MATERIAL_GLTF_PBR))
    {
        float gltf_mode = pc.material_modes.y;
        if (gltf_mode < 0.5)
        {
            color.a = 1.0;
        }
        else if (gltf_mode > 1.5)
        {
            if (pc.params.x >= 0.0 && color.a < pc.params.x)
            {
                discard;
            }
            color.a = 1.0;
        }
        return color;
    }

    float diffuse_mode = pc.material_modes.x;
    if (diffuse_mode < 0.5)
    {
        color.a = 1.0;
    }
    else if (diffuse_mode > 1.5 && diffuse_mode < 2.5)
    {
        if (pc.params.x >= 0.0 && color.a < pc.params.x)
        {
            discard;
        }
        color.a = 1.0;
    }
    else if (diffuse_mode > 2.5 && diffuse_mode < 3.5)
    {
        color.rgb += color.rgb * color.a;
        color.a = 1.0;
    }
    return color;
}

vec3 material_normal(vec2 texcoord)
{
    vec3 n = normalize(vary_normal);
    if (has_material_flag(MATERIAL_DOUBLE_SIDED) && !gl_FrontFacing)
    {
        n = -n;
    }
    if (dot(n, n) <= 0.0001)
    {
        n = vec3(0.0, 0.0, 1.0);
    }

    if (has_material_flag(MATERIAL_HAS_NORMAL_MAP) ||
        has_material_flag(MATERIAL_LEGACY_BUMP))
    {
        vec3 t = normalize(vary_tangent.xyz);
        if (dot(t, t) <= 0.0001)
        {
            t = vec3(1.0, 0.0, 0.0);
        }

        vec3 b = normalize(cross(n, t) * vary_tangent.w);
        vec3 map_normal = texture(tex1, normal_sample_texcoord(texcoord)).xyz * 2.0 - 1.0;
        n = normalize(mat3(t, b, n) * map_normal);
    }

    return n;
}

vec3 alpha_lighting(vec3 base_color, vec2 texcoord)
{
    if (has_material_flag(MATERIAL_FULLBRIGHT))
    {
        return base_color;
    }

    float roughness = clamp(pc.material_pbr.x, 0.04, 1.0);
    float metallic = clamp(pc.material_pbr.y, 0.0, 1.0);
    float occlusion = 1.0;
    if (has_material_flag(MATERIAL_HAS_ORM_MAP))
    {
        vec3 orm = texture(tex2, orm_texture_texcoord(texcoord)).rgb;
        occlusion = clamp(orm.r, 0.0, 1.0);
        roughness = clamp(orm.g * pc.material_pbr.x, 0.04, 1.0);
        metallic = clamp(orm.b * pc.material_pbr.y, 0.0, 1.0);
    }

    vec3 n = material_normal(texcoord);
    vec3 light_dir = get_scene_light_direction();
    vec3 view_dir = vec3(0.0, 0.0, 1.0);
    float ndotl = max(dot(n, light_dir), 0.0);
    vec3 ambient = base_color * get_scene_ambient_color(0.42) * occlusion;
    vec3 direct_color = get_scene_direct_color();
    vec3 direct = base_color * direct_color * ndotl * mix(0.92, 0.58, metallic);
    float spec_power = mix(64.0, 10.0, roughness);
    vec3 specular_color = mix(vec3(0.05), pc.material_legacy.rgb, metallic);
    float specular_scale = 1.0;
    if (has_material_flag(MATERIAL_HAS_SPECULAR_MAP) &&
        !has_material_flag(MATERIAL_HAS_ORM_MAP))
    {
        vec4 legacy_spec = texture(tex2, legacy_specular_sample_texcoord());
        specular_color = legacy_spec.rgb * pc.material_legacy.rgb;
        specular_scale = max(legacy_spec.a, 0.15);
    }
    if (has_material_flag(MATERIAL_LEGACY_SHINY))
    {
        specular_scale *= 1.0 + clamp(pc.material_modes.w, 0.0, 3.0) * 0.25;
    }

    float specular = pow(max(dot(reflect(-light_dir, n), view_dir), 0.0), spec_power) *
        (1.0 - roughness) *
        specular_scale;
    vec3 environment = base_color * clamp(pc.material_legacy.a, 0.0, 1.0) * 0.16;

    return ambient + direct + specular_color * specular * direct_color + environment;
}

void clip_against_scene_depth()
{
    if (!has_material_flag(MATERIAL_HAS_SCENE_DEPTH))
    {
        return;
    }

    vec2 depth_size = max(vec2(textureSize(sceneDepthMap, 0)), vec2(1.0));
    vec2 screen_texcoord = clamp(gl_FragCoord.xy / depth_size, vec2(0.0), vec2(1.0));
    if (has_material_flag(MATERIAL_SCENE_DEPTH_FLIP_Y))
    {
        screen_texcoord.y = 1.0 - screen_texcoord.y;
    }
    float scene_depth = texture(sceneDepthMap, screen_texcoord).r;
    if (has_material_flag(MATERIAL_SCENE_DEPTH_REVERSED))
    {
        if (scene_depth > 0.00001 && gl_FragCoord.z < scene_depth - 0.00001)
        {
            discard;
        }
    }
    else if (scene_depth < 0.99999 && gl_FragCoord.z > scene_depth + 0.00001)
    {
        discard;
    }
}

void main()
{
    vec4 color =
        diffuse_lookup(vary_texcoord0.xy) *
        vertex_color *
        vec4(pc.material_params.rgb, pc.material_pbr.w);
    color = apply_alpha_policy(color);

    clip_against_scene_depth();

    color.rgb = alpha_lighting(color.rgb, vary_material_texcoord0.xy);

    vec3 emissive = pc.material_extra.rgb;
    if (pc.material_extra.a > 0.5)
    {
        emissive *= texture(tex3, emissive_texture_texcoord(vary_material_texcoord0.xy)).rgb;
    }
    color.rgb += emissive;

    if (has_material_flag(MATERIAL_GLOW))
    {
        color.rgb += color.rgb * 0.35;
    }

    frag_color = color;
}
