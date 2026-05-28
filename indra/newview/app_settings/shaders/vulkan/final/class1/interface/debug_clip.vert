#version 450

layout(push_constant) uniform MareInterfacePushConstants
{
    mat4 modelview_projection_matrix;
    mat4 texture_matrix0;
    mat4 modelview_matrix;
} pc;

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 vary_position;

void main()
{
    vary_position = (pc.modelview_matrix * vec4(position, 1.0)).xyz;
    gl_Position = pc.modelview_projection_matrix * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;
}
