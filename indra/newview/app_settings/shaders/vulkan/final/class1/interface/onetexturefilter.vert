#version 450

layout(push_constant) uniform MareInterfacePushConstants
{
    mat4 modelview_projection_matrix;
} pc;

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord0;

layout(location = 0) out vec2 vary_texcoord0;

void main()
{
    gl_Position = pc.modelview_projection_matrix * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;
    vary_texcoord0 = texcoord0;
}
