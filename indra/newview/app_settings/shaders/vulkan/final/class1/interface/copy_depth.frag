#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 1) uniform sampler2D depthMap;

layout(location = 0) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = texture(diffuseMap, vary_texcoord0);
    gl_FragDepth = texture(depthMap, vary_texcoord0).r;
}
