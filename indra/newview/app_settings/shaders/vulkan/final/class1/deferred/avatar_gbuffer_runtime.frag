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

layout(location = 0) out vec4 frag_diffuse;
layout(location = 1) out vec4 frag_specular;
layout(location = 2) out vec4 frag_normal;

const uint MATERIAL_AVATAR_IMPOSTOR = 65536u;
const float GBUFFER_FLAG_HAS_ATMOS = 0.34;

bool has_material_flag(uint flag)
{
    return (uint(pc.material_pbr.z + 0.5) & flag) != 0u;
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

void main()
{
    vec4 color =
        diffuse_lookup(vary_texcoord0.xy) *
        vertex_color *
        vec4(pc.material_params.rgb, pc.material_pbr.w);

    if (pc.params.x >= 0.0 && color.a < pc.params.x)
    {
        discard;
    }

    vec3 n = normalize(vary_normal);
    if (dot(n, n) <= 0.0001)
    {
        n = vec3(0.0, 0.0, 1.0);
    }
    vec3 encoded_normal = n * 0.5 + 0.5;

    frag_diffuse = vec4(max(color.rgb, vec3(0.0)), pc.material_legacy.a);
    if (has_material_flag(MATERIAL_AVATAR_IMPOSTOR))
    {
        vec4 impostor_normal = texture(tex1, vary_texcoord0.xy);
        frag_specular = texture(tex2, vary_texcoord0.xy);
        frag_normal = vec4(clamp(impostor_normal.xyz, vec3(0.0), vec3(1.0)), GBUFFER_FLAG_HAS_ATMOS);
    }
    else
    {
        frag_specular = vec4(0.04, 0.04, 0.04, 0.0);
        frag_normal = vec4(encoded_normal.xyz, GBUFFER_FLAG_HAS_ATMOS);
    }
}
