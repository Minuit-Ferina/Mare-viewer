// Vulkan final shader source port.
// Source OpenGL shader: class1/deferred/shadowAlphaMaskF.glsl

#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
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
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 4) flat in uint vary_texture_index;
layout(location = 8) in vec4 vertex_position;

layout(location = 0) out vec4 frag_color;

vec4 sample_indexed_texture(vec2 texcoord)
{
    switch (vary_texture_index)
    {
        case 1u: return texture(tex1, texcoord);
        case 2u: return texture(tex2, texcoord);
        case 3u: return texture(tex3, texcoord);
        case 4u: return texture(tex4, texcoord);
        case 5u: return texture(tex5, texcoord);
        case 6u: return texture(tex6, texcoord);
        case 7u: return texture(tex7, texcoord);
        default: return texture(diffuseMap, texcoord);
    }
}

void main()
{
    float alpha = sample_indexed_texture(vary_texcoord0.xy).a * vertex_color.a;

    if (alpha < pc.params.x)
    {
        discard;
    }

    if (alpha < 0.05)
    {
        discard;
    }

    if (alpha < 0.88 &&
        fract(0.5 * floor(vertex_position.x / vertex_position.w)) < 0.25)
    {
        discard;
    }

    frag_color = vec4(1.0);
}
