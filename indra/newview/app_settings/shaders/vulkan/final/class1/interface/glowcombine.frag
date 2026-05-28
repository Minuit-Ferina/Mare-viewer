#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D emissiveRect;

layout(location = 0) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color =
        texture(diffuseRect, vary_texcoord0) +
        texture(emissiveRect, vary_texcoord0);
}
