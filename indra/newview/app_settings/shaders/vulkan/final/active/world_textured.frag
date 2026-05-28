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
    layout(offset = 176) vec4 material_pbr;
    layout(offset = 192) vec4 material_legacy;
    layout(offset = 208) vec4 material_modes;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 2) flat in uint vary_texture_index;
layout(location = 3) in vec3 vary_normal;
layout(location = 4) in vec4 vary_tangent;

layout(location = 0) out vec4 frag_color;

const uint MATERIAL_HAS_NORMAL_MAP = 1u;
const uint MATERIAL_HAS_ORM_MAP = 2u;
const uint MATERIAL_FULLBRIGHT = 4u;
const uint MATERIAL_GLOW = 8u;
const uint MATERIAL_WATER = 16u;
const uint MATERIAL_HAS_SPECULAR_MAP = 32u;
const uint MATERIAL_ALPHA_BLEND = 64u;
const uint MATERIAL_ALPHA_MASK = 128u;
const uint MATERIAL_DOUBLE_SIDED = 256u;
const uint MATERIAL_LEGACY_BUMP = 512u;
const uint MATERIAL_LEGACY_SHINY = 1024u;
const uint MATERIAL_GLTF_PBR = 2048u;
const uint MATERIAL_POST_DEFERRED = 4096u;

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
        vec3 map_normal = texture(tex1, texcoord).xyz * 2.0 - 1.0;
        n = normalize(mat3(t, b, n) * map_normal);
    }

    return n;
}

vec3 final_lighting(vec3 base_color, vec2 texcoord)
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
        vec3 orm = texture(tex2, texcoord).rgb;
        occlusion = clamp(orm.r, 0.0, 1.0);
        roughness = clamp(orm.g * pc.material_pbr.x, 0.04, 1.0);
        metallic = clamp(orm.b * pc.material_pbr.y, 0.0, 1.0);
    }

    vec3 n = material_normal(texcoord);
    vec3 light_dir = normalize(vec3(0.35, 0.45, 0.82));
    vec3 view_dir = vec3(0.0, 0.0, 1.0);
    float ndotl = max(dot(n, light_dir), 0.0);
    float ambient = 0.36 * occlusion;
    float diffuse = ambient + ndotl * mix(1.0, 0.58, metallic);
    float spec_power = mix(72.0, 10.0, roughness);
    vec3 specular_color = mix(vec3(0.05), pc.material_legacy.rgb, metallic);
    float specular_scale = 1.0;
    if (has_material_flag(MATERIAL_HAS_SPECULAR_MAP) &&
        !has_material_flag(MATERIAL_HAS_ORM_MAP))
    {
        vec4 legacy_spec = texture(tex2, texcoord);
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
    vec3 environment = base_color * clamp(pc.material_legacy.a, 0.0, 1.0) * 0.18;

    return base_color * diffuse + specular_color * specular + environment;
}

void main()
{
    vec4 color =
        diffuse_lookup(vary_texcoord0.xy) *
        vertex_color *
        vec4(pc.material_params.rgb, pc.material_pbr.w);

    if (has_material_flag(MATERIAL_WATER))
    {
        color.rgb = mix(color.rgb, vec3(0.18, 0.42, 0.58), 0.28);
        color.a *= 0.78;
    }

    color.rgb = final_lighting(color.rgb, vary_texcoord0.xy);

    vec3 emissive = pc.material_extra.rgb;
    if (pc.material_extra.a > 0.5)
    {
        emissive *= texture(tex3, vary_texcoord0.xy).rgb;
    }
    color.rgb += emissive;
    if (has_material_flag(MATERIAL_GLOW))
    {
        color.rgb += color.rgb * 0.45;
    }

    if (pc.params.x >= 0.0 && color.a < pc.params.x)
    {
        discard;
    }
    frag_color = color;
}
