#version 450

layout(set = 1, binding = 0) uniform MareSimpleUniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    vec4 color;
} u;

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec4 vertex_position;

void main()
{
    vertex_position = u.modelview_projection_matrix * vec4(position.xyz, 1.0);
    gl_Position = vertex_position;
    gl_Position.y = -gl_Position.y;
    vertex_color = u.color;
}
