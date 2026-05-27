#version 450

layout(push_constant) uniform MareWorldPushConstants
{
    mat4 modelview_projection_matrix;
    vec4 params;
    vec4 terrain_params;
    vec4 texture_transform_s;
    vec4 texture_transform_t;
} pc;

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord0;
layout(location = 6) in vec4 diffuse_color;
layout(location = 13) in uint texture_index;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;
layout(location = 2) flat out uint vary_texture_index;

void main()
{
    gl_Position = pc.modelview_projection_matrix * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;
    vec4 tc = vec4(texcoord0, 0.0, 1.0);
    vary_texcoord0 = vec2(
        dot(tc, pc.texture_transform_s),
        dot(tc, pc.texture_transform_t));
    vary_texture_index = texture_index;
    vertex_color = diffuse_color;
}
