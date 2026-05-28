#version 450

layout(set = 1, binding = 0) uniform MareDiffuseUniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 texture_matrix0;
    mat4 normal_matrix;
} u;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;
layout(location = 6) in vec4 diffuse_color;

layout(location = 0) out vec3 vary_normal;
layout(location = 1) out vec4 vertex_color;
layout(location = 2) out vec2 vary_texcoord0;
layout(location = 3) out vec3 vary_position;

void main()
{
    vary_position = (u.modelview_matrix * vec4(position.xyz, 1.0)).xyz;
    gl_Position = u.modelview_projection_matrix * vec4(position.xyz, 1.0);
    gl_Position.y = -gl_Position.y;
    vary_normal = normalize((u.normal_matrix * vec4(normal, 0.0)).xyz);
    vary_texcoord0 = (u.texture_matrix0 * vec4(texcoord0, 0.0, 1.0)).xy;
    vertex_color = diffuse_color;
}
