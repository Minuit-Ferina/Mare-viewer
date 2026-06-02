#version 450

layout(set = 1, binding = 0) uniform MarePointLightVertexUniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    vec4 center_size;
} u;

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 vary_fragcoord;
layout(location = 1) out vec3 trans_center;

void main()
{
    vec3 p = position * u.center_size.w + u.center_size.xyz;
    vec4 pos = u.modelview_projection_matrix * vec4(p.xyz, 1.0);

    vary_fragcoord = pos;
    trans_center = (u.modelview_matrix * vec4(u.center_size.xyz, 1.0)).xyz;

    gl_Position = pos;
    gl_Position.y = -gl_Position.y;
}
