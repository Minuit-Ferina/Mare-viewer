#version 450

layout(push_constant) uniform MareInterfacePushConstants
{
    mat4 modelview_projection_matrix;
    mat4 texture_matrix0;
} pc;

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord0;
layout(location = 6) in vec4 diffuse_color;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;

void main()
{
    gl_Position = pc.modelview_projection_matrix * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;
    vary_texcoord0 = (pc.texture_matrix0 * vec4(texcoord0, 0.0, 1.0)).xy;
    vertex_color = diffuse_color;
}
