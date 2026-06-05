#version 450

layout(std140, set = 3, binding = 0) uniform MareWorldPushConstants
{
    mat4 modelview_projection_matrix;
    vec4 params;
    vec4 terrain_params;
    vec4 texture_transform_s;
    vec4 texture_transform_t;
    vec4 material_extra;
    vec4 base_texture_transform0;
    vec4 base_texture_transform1;
    layout(offset = 352) mat4 normal_matrix;
} pc;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;
layout(location = 3) in vec2 texcoord1;
layout(location = 4) in vec2 texcoord2;
layout(location = 6) in vec4 diffuse_color;
layout(location = 8) in vec4 tangent;
layout(location = 13) in uint texture_index;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;
layout(location = 2) flat out uint vary_texture_index;
layout(location = 3) out vec3 vary_normal;
layout(location = 4) out vec4 vary_tangent;
layout(location = 5) out vec3 vary_position;
layout(location = 6) out vec2 vary_material_texcoord0;
layout(location = 7) out vec2 vary_material_texcoord1;
layout(location = 8) out vec2 vary_material_texcoord2;

void main()
{
    gl_Position = pc.modelview_projection_matrix * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;

    vertex_color = diffuse_color;
    vary_texcoord0 = texcoord0;
    vary_texture_index = texture_index;
    vary_normal = normalize((pc.normal_matrix * vec4(normal, 0.0)).xyz);
    vary_tangent = vec4(normalize((pc.normal_matrix * vec4(tangent.xyz, 0.0)).xyz), tangent.w);
    vary_position = position;
    vary_material_texcoord0 = texcoord0;
    vary_material_texcoord1 = texcoord1;
    vary_material_texcoord2 = texcoord2;
}
