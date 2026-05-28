#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform MareInterfacePushConstants
{
    layout(offset = 128) vec4 color;
} pc;

layout(location = 0) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = max(pc.color * texture(diffuseMap, vary_texcoord0), vec4(0.0));
}
