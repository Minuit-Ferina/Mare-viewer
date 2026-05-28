#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform MareInterfacePushConstants
{
    layout(offset = 128) vec4 color;
    vec4 clip_plane;
    vec4 params;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    vec4 color = vertex_color * texture(diffuseMap, vary_texcoord0);
    if (color.a < pc.params.x)
    {
        discard;
    }

    frag_color = max(color, vec4(0.0));
}
