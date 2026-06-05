// Vulkan final shader source port.
// Source OpenGL shader: class1/deferred/shadowSkinnedV.glsl
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
    layout(offset = 352) mat4 normal_matrix;
} pc;

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

layout(std430, set = 0, binding = 17) readonly buffer MareSkinningPalette
{
    vec4 matrix_palette[];
} skinning;

vec3 skin_position(vec3 source_position)
{
    uint matrix_count = uint(pc.params.w + 0.5);
    if (matrix_count == 0u)
    {
        return source_position;
    }

    uint matrix_offset = uint(pc.params.z + 0.5);
    if (pc.terrain_params.w > 0.5)
    {
        uint max_index = matrix_count - 1u;
        uint joint_index = min(uint(max(floor(weight), 0.0)), max_index);
        uint next_joint_index = min(joint_index + 1u, max_index);
        float mix_value = fract(weight);
        uint base0 = (matrix_offset + joint_index) * 3u;
        uint base1 = (matrix_offset + next_joint_index) * 3u;

        vec4 row0 = mix(skinning.matrix_palette[base0 + 0u], skinning.matrix_palette[base1 + 0u], mix_value);
        vec4 row1 = mix(skinning.matrix_palette[base0 + 1u], skinning.matrix_palette[base1 + 1u], mix_value);
        vec4 row2 = mix(skinning.matrix_palette[base0 + 2u], skinning.matrix_palette[base1 + 2u], mix_value);
        vec4 pos = vec4(source_position, 1.0);
        return vec3(dot(row0, pos), dot(row1, pos), dot(row2, pos));
    }

    vec4 indices = floor(weight4);
    vec4 weights = fract(weight4);
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

void main()
{
    vec4 object_position = vec4(skin_position(position.xyz), 1.0);
    vec4 clip_position = pc.modelview_projection_matrix * object_position;
    vec4 texcoord = vec4(texcoord0, 0.0, 1.0);

    gl_Position = clip_position;
    gl_Position.y = -gl_Position.y;

    vary_position = object_position.xyz;
    vary_normal = normalize((pc.normal_matrix * vec4(normal, 0.0)).xyz);
    vary_texcoord0 = vec2(dot(pc.texture_transform_s, texcoord), dot(pc.texture_transform_t, texcoord));
    vary_texcoord1 = texcoord1;
    vary_texcoord2 = texcoord2;
    vary_texture_index = texture_index;
    vary_tangent = tangent;
    vertex_position = clip_position;
    vertex_color = diffuse_color;
}
