#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D tex2;
layout(set = 0, binding = 3) uniform sampler2D tex3;
layout(set = 0, binding = 4) uniform sampler2D tex4;
layout(set = 0, binding = 5) uniform sampler2D tex5;
layout(set = 0, binding = 6) uniform sampler2D tex6;
layout(set = 0, binding = 7) uniform sampler2D tex7;

layout(push_constant) uniform MareDiffuseMaskPushConstants
{
    float minimum_alpha;
} pc;

layout(location = 0) in vec3 vary_normal;
layout(location = 1) in vec4 vertex_color;
layout(location = 2) in vec2 vary_texcoord0;
layout(location = 3) in vec3 vary_position;
layout(location = 4) flat in uint vary_texture_index;

layout(location = 0) out vec4 frag_data[4];

vec4 encode_normal(vec3 n, float env, float gbuffer_flag)
{
    vec3 encoded = normalize(n) * 0.5 + 0.5;
    return vec4(encoded.xy, env, gbuffer_flag);
}

vec4 diffuse_lookup(vec2 texcoord)
{
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
    vec4 color = diffuse_lookup(vary_texcoord0.xy) * vertex_color;

    if (color.a < pc.minimum_alpha)
    {
        discard;
    }

    frag_data[0] = vec4(color.rgb, 0.0);
    frag_data[1] = vec4(0.0);
    frag_data[2] = encode_normal(vary_normal, 0.0, 1.0);
    frag_data[3] = vec4(0.0);
}
