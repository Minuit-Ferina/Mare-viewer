// Vulkan final shader source port.
// Source OpenGL shader: class1/deferred/bumpV.glsl
// This file preserves the source shader's role while the final Vulkan
// renderer pipeline contracts are completed.

#version 450

layout(set = 1, binding = 0) uniform MareGeneratedVertexUniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 texture_matrix0;
    mat4 texture_matrix1;
    mat4 texture_matrix2;
    mat4 normal_matrix;
    vec4 color;
    vec4 object_plane_s;
    vec4 object_plane_t;
    vec4 params;
} u;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;
layout(location = 3) in vec2 texcoord1;
layout(location = 4) in vec2 texcoord2;
layout(location = 6) in vec4 diffuse_color;
layout(location = 8) in vec4 tangent;
layout(location = 9) in float weight;
layout(location = 10) in vec4 weight4;
layout(location = 13) in uint texture_index;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;
layout(location = 2) out vec3 vary_normal;
layout(location = 3) out vec3 vary_position;
layout(location = 4) flat out uint vary_texture_index;
layout(location = 5) out vec4 vary_tangent;
layout(location = 6) out vec2 vary_texcoord1;
layout(location = 7) out vec2 vary_texcoord2;
layout(location = 8) out vec4 vertex_position;

void main()
{
    vec4 object_position = vec4(position.xyz, 1.0);
    vec4 eye_position = u.modelview_matrix * object_position;
    vec4 clip_position = u.modelview_projection_matrix * object_position;

    gl_Position = clip_position;
    gl_Position.y = -gl_Position.y;

    vary_position = eye_position.xyz;
    vary_normal = normalize((u.normal_matrix * vec4(normal, 0.0)).xyz);
    vary_texcoord0 = (u.texture_matrix0 * vec4(texcoord0, 0.0, 1.0)).xy;
    vary_texcoord1 = (u.texture_matrix1 * vec4(texcoord1, 0.0, 1.0)).xy;
    vary_texcoord2 = (u.texture_matrix2 * vec4(texcoord2, 0.0, 1.0)).xy;
    vary_texture_index = texture_index;
    vary_tangent = tangent;
    vertex_position = clip_position;
    vertex_color = diffuse_color;
}
