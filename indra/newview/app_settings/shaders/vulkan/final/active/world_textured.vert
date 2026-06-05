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
layout(location = 2) flat out uint vary_texture_index;
layout(location = 3) out vec3 vary_normal;
layout(location = 4) out vec4 vary_tangent;
layout(location = 5) out vec3 vary_position;
layout(location = 6) out vec2 vary_material_texcoord0;
layout(location = 7) out vec2 vary_material_texcoord1;
layout(location = 8) out vec2 vary_material_texcoord2;

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

        vec4 row0 = mix(
            skinning.matrix_palette[base0 + 0u],
            skinning.matrix_palette[base1 + 0u],
            mix_value);
        vec4 row1 = mix(
            skinning.matrix_palette[base0 + 1u],
            skinning.matrix_palette[base1 + 1u],
            mix_value);
        vec4 row2 = mix(
            skinning.matrix_palette[base0 + 2u],
            skinning.matrix_palette[base1 + 2u],
            mix_value);
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

        uint matrix_index =
            matrix_offset + min(uint(max(indices[i], 0.0)), max_index);
        uint column_base = matrix_index * 3u;
        vec4 column0 = skinning.matrix_palette[column_base + 0u];
        vec4 column1 = skinning.matrix_palette[column_base + 1u];
        vec4 column2 = skinning.matrix_palette[column_base + 2u];
        vec3 transformed = vec3(
            column0.x * source_position.x +
                column1.x * source_position.y +
                column2.x * source_position.z +
                column0.w,
            column0.y * source_position.x +
                column1.y * source_position.y +
                column2.y * source_position.z +
                column1.w,
            column0.z * source_position.x +
                column1.z * source_position.y +
                column2.z * source_position.z +
                column2.w);
        skinned_position += transformed * joint_weight;
    }

    return skinned_position;
}

vec3 skin_direction(vec3 source_direction)
{
    uint matrix_count = uint(pc.params.w + 0.5);
    if (matrix_count == 0u)
    {
        return source_direction;
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

        vec4 row0 = mix(
            skinning.matrix_palette[base0 + 0u],
            skinning.matrix_palette[base1 + 0u],
            mix_value);
        vec4 row1 = mix(
            skinning.matrix_palette[base0 + 1u],
            skinning.matrix_palette[base1 + 1u],
            mix_value);
        vec4 row2 = mix(
            skinning.matrix_palette[base0 + 2u],
            skinning.matrix_palette[base1 + 2u],
            mix_value);
        return vec3(
            dot(row0.xyz, source_direction),
            dot(row1.xyz, source_direction),
            dot(row2.xyz, source_direction));
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
    vec3 skinned_direction = vec3(0.0);
    for (uint i = 0u; i < 4u; ++i)
    {
        float joint_weight = weights[i];
        if (joint_weight <= 0.0)
        {
            continue;
        }

        uint matrix_index =
            matrix_offset + min(uint(max(indices[i], 0.0)), max_index);
        uint column_base = matrix_index * 3u;
        vec4 column0 = skinning.matrix_palette[column_base + 0u];
        vec4 column1 = skinning.matrix_palette[column_base + 1u];
        vec4 column2 = skinning.matrix_palette[column_base + 2u];
        vec3 transformed = vec3(
            column0.x * source_direction.x +
                column1.x * source_direction.y +
                column2.x * source_direction.z,
            column0.y * source_direction.x +
                column1.y * source_direction.y +
                column2.y * source_direction.z,
            column0.z * source_direction.x +
                column1.z * source_direction.y +
                column2.z * source_direction.z);
        skinned_direction += transformed * joint_weight;
    }

    if (dot(skinned_direction, skinned_direction) <= 0.0001)
    {
        return source_direction;
    }
    return skinned_direction;
}

vec2 khr_texture_transform(vec2 texcoord, vec2 scale, float rotation, vec2 offset)
{
    mat3 scale_mat = mat3(scale.x, 0.0, 0.0, 0.0, scale.y, 0.0, 0.0, 0.0, 1.0);
    mat3 offset_mat = mat3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, offset.x, offset.y, 1.0);
    mat3 rotation_mat = mat3(
        cos(rotation), -sin(rotation), 0.0,
        sin(rotation),  cos(rotation), 0.0,
        0.0,            0.0,           1.0);
    return (offset_mat * rotation_mat * scale_mat * vec3(texcoord, 1.0)).xy;
}

vec2 base_texture_transform(vec2 texcoord)
{
    texcoord.y = 1.0 - texcoord.y;
    texcoord = khr_texture_transform(
        texcoord,
        pc.base_texture_transform0.xy,
        pc.base_texture_transform0.z,
        vec2(pc.base_texture_transform0.w, pc.base_texture_transform1.x));
    texcoord.y = 1.0 - texcoord.y;
    return texcoord;
}

vec2 texture_matrix_transform(vec2 texcoord)
{
    vec4 tc = vec4(texcoord, 0.0, 1.0);
    return vec2(
        dot(tc, pc.texture_transform_s),
        dot(tc, pc.texture_transform_t));
}

vec3 safe_normalize(vec3 value, vec3 fallback)
{
    if (dot(value, value) <= 0.0001)
    {
        return fallback;
    }
    return normalize(value);
}

void main()
{
    vec3 skinned_position = skin_position(position);
    vec3 skinned_normal = skin_direction(normal);
    vec3 skinned_tangent = skin_direction(tangent.xyz);
    vec3 transformed_normal =
        safe_normalize(mat3(pc.normal_matrix) * skinned_normal, vec3(0.0, 0.0, 1.0));
    vec3 transformed_tangent =
        safe_normalize(mat3(pc.normal_matrix) * skinned_tangent, vec3(1.0, 0.0, 0.0));

    gl_Position = pc.modelview_projection_matrix * vec4(skinned_position, 1.0);
    gl_Position.y = -gl_Position.y;
    vec2 material_texcoord = texture_matrix_transform(texcoord0);
    vary_texcoord0 = base_texture_transform(material_texcoord);
    vary_material_texcoord0 = material_texcoord;
    vary_material_texcoord1 = texture_matrix_transform(texcoord1);
    vary_material_texcoord2 = texture_matrix_transform(texcoord2);
    vary_texture_index = texture_index;
    vary_normal = transformed_normal;
    vary_tangent = vec4(transformed_tangent, tangent.w);
    vary_position = skinned_position;
    vertex_color = diffuse_color;
}
