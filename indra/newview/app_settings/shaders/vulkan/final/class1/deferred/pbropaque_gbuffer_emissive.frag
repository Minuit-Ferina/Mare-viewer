#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D tex2;
layout(set = 0, binding = 3) uniform sampler2D tex3;
layout(set = 0, binding = 4) uniform sampler2D tex4;
layout(set = 0, binding = 5) uniform sampler2D tex5;
layout(set = 0, binding = 6) uniform sampler2D tex6;
layout(set = 0, binding = 7) uniform sampler2D tex7;

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
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 2) flat in uint vary_texture_index;
layout(location = 3) in vec3 vary_normal;
layout(location = 4) in vec4 vary_tangent;
layout(location = 6) in vec2 vary_material_texcoord0;

layout(location = 0) out vec4 frag_diffuse;
layout(location = 1) out vec4 frag_orm;
layout(location = 2) out vec4 frag_normal;
layout(location = 3) out vec4 frag_emissive;

const uint MATERIAL_HAS_NORMAL_MAP = 1u;
const uint MATERIAL_HAS_ORM_MAP = 2u;
const uint MATERIAL_DOUBLE_SIDED = 256u;
const float GBUFFER_FLAG_HAS_PBR = 0.67;

bool has_material_flag(uint flag)
{
    return (uint(pc.material_pbr.z + 0.5) & flag) != 0u;
}

vec3 srgb_to_linear(vec3 color)
{
    bvec3 cutoff = lessThanEqual(color, vec3(0.04045));
    vec3 low = color / 12.92;
    vec3 high = pow((color + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
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

vec4 apply_gltf_alpha_policy(vec4 color)
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

    if (has_material_flag(MATERIAL_HAS_NORMAL_MAP))
    {
        vec3 t = normalize(vary_tangent.xyz);
        if (dot(t, t) <= 0.0001)
        {
            t = vec3(1.0, 0.0, 0.0);
        }
        vec3 b = normalize(cross(n, t) * vary_tangent.w);
        vec3 map_normal = texture(tex1, normal_texture_texcoord(texcoord)).xyz * 2.0 - 1.0;
        n = normalize(mat3(t, b, n) * map_normal);
    }

    return n;
}

void main()
{
    vec4 color =
        diffuse_lookup(vary_texcoord0.xy) *
        vertex_color *
        vec4(pc.material_params.rgb, pc.material_pbr.w);

    color = apply_gltf_alpha_policy(color);
    color.rgb = srgb_to_linear(max(color.rgb, vec3(0.0)));

    vec3 orm = vec3(1.0, clamp(pc.material_pbr.x, 0.04, 1.0), clamp(pc.material_pbr.y, 0.0, 1.0));
    if (has_material_flag(MATERIAL_HAS_ORM_MAP))
    {
        orm = texture(tex2, orm_texture_texcoord(vary_material_texcoord0.xy)).rgb;
        orm.g *= pc.material_pbr.x;
        orm.b *= pc.material_pbr.y;
    }

    vec3 emissive = pc.material_extra.rgb;
    if (pc.material_extra.a > 0.5)
    {
        emissive *= texture(tex3, emissive_texture_texcoord(vary_material_texcoord0.xy)).rgb;
    }
    emissive = srgb_to_linear(max(emissive, vec3(0.0)));

    vec3 pbr_normal = normalize(material_normal(vary_material_texcoord0.xy));
    float encoded_normal_f = sqrt(8.0 * pbr_normal.z + 8.0);
    frag_diffuse = vec4(color.rgb, pc.material_legacy.a);
    frag_orm = vec4(max(orm, vec3(0.0)), 1.0);
    frag_normal = vec4(pbr_normal.xy / encoded_normal_f + 0.5, 0.0, GBUFFER_FLAG_HAS_PBR);
    frag_emissive = vec4(emissive, 0.0);
}
