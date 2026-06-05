// Vulkan final shader source port.
// Source OpenGL shader: class1/deferred/pbrShadowAlphaMaskV.glsl
// This file preserves the source shader's role while the final Vulkan
// renderer pipeline contracts are completed.

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
    layout(offset = 176) vec4 material_pbr;
    layout(offset = 352) mat4 normal_matrix;
} pc;

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord0;
layout(location = 6) in vec4 diffuse_color;
layout(location = 10) in vec4 weight4;
layout(location = 12) in uvec4 joint;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;
layout(location = 8) out vec4 vertex_position;

layout(std430, set = 0, binding = 17) readonly buffer MareSkinningPalette
{
    vec4 matrix_palette[];
} skinning;

const uint MATERIAL_GLTF_PBR = 2048u;

bool has_material_flag(uint flag)
{
    return (uint(pc.material_pbr.z + 0.5) & flag) != 0u;
}

vec3 skin_position(vec3 source_position)
{
    uint matrix_count = uint(pc.params.w + 0.5);
    if (matrix_count == 0u)
    {
        return source_position;
    }

    uint matrix_offset = uint(pc.params.z + 0.5);
    bool gltf_skinning = has_material_flag(MATERIAL_GLTF_PBR);
    vec4 indices = gltf_skinning ? vec4(joint) : floor(weight4);
    vec4 weights = gltf_skinning ? weight4 : fract(weight4);
    float weight_sum = weights.x + weights.y + weights.z + weights.w;
    if (weight_sum <= 0.0)
    {
        weights = vec4(1.0, 0.0, 0.0, 0.0);
        weight_sum = 1.0;
    }
    weights /= weight_sum;

    uint max_index = matrix_count - 1u;
    vec3 skinned_position = vec3(0.0);
    for (uint i = 0u; i < 4u; ++i)
    {
        float joint_weight = weights[i];
        if (joint_weight <= 0.0)
        {
            continue;
        }

        uint matrix_index = matrix_offset + min(uint(max(indices[i], 0.0)), max_index);
        uint column_base = matrix_index * 3u;
        vec4 column0 = skinning.matrix_palette[column_base + 0u];
        vec4 column1 = skinning.matrix_palette[column_base + 1u];
        vec4 column2 = skinning.matrix_palette[column_base + 2u];
        vec3 transformed = vec3(
            column0.x * source_position.x + column1.x * source_position.y + column2.x * source_position.z + column0.w,
            column0.y * source_position.x + column1.y * source_position.y + column2.y * source_position.z + column1.w,
            column0.z * source_position.x + column1.z * source_position.y + column2.z * source_position.z + column2.w);
        skinned_position += transformed * joint_weight;
    }

    return skinned_position;
}

vec2 khr_texture_transform(vec2 texcoord, vec2 scale, float rotation, vec2 offset)
{
    mat3 scale_mat = mat3(scale.x, 0.0, 0.0, 0.0, scale.y, 0.0, 0.0, 0.0, 1.0);
    mat3 offset_mat = mat3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, offset.x, offset.y, 1.0);
    mat3 rotation_mat = mat3(
        cos(rotation), -sin(rotation), 0.0,
        sin(rotation),  cos(rotation), 0.0,
        0.0,           0.0,          1.0);

    return (offset_mat * rotation_mat * scale_mat * vec3(texcoord, 1.0)).xy;
}

vec2 base_color_texture_transform(vec2 vertex_texcoord)
{
    vec4 texcoord = vec4(vertex_texcoord, 0.0, 1.0);
    vec2 transformed = vec2(
        dot(pc.texture_transform_s, texcoord),
        dot(pc.texture_transform_t, texcoord));

    transformed.y = 1.0 - transformed.y;
    transformed = khr_texture_transform(
        transformed,
        pc.base_texture_transform0.xy,
        pc.base_texture_transform0.z,
        vec2(pc.base_texture_transform0.w, pc.base_texture_transform1.x));
    transformed.y = 1.0 - transformed.y;
    return transformed;
}

void main()
{
    vec4 object_position = vec4(skin_position(position.xyz), 1.0);
    vec4 clip_position = pc.modelview_projection_matrix * object_position;

    gl_Position = clip_position;
    gl_Position.y = -gl_Position.y;

    vary_texcoord0 = base_color_texture_transform(texcoord0);
    vertex_position = clip_position;
    vertex_color = diffuse_color;
}
