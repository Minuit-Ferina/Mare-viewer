#version 450

layout(std140, set = 3, binding = 0) uniform MareWorldPushConstants
{
    mat4 modelview_projection_matrix;
    vec4 params;
    vec4 terrain_params;
    layout(offset = 272) vec4 terrain_texture_transform0;
    layout(offset = 288) vec4 terrain_texture_transform1;
    layout(offset = 304) vec4 terrain_texture_transform2;
    layout(offset = 320) vec4 terrain_texture_transform3;
    layout(offset = 336) vec4 terrain_texture_transform4;
    layout(offset = 352) mat4 normal_matrix;
} pc;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 texcoord1;
layout(location = 8) in vec4 tangent;

layout(location = 0) out vec2 vary_detail_texcoord0;
layout(location = 1) out vec2 vary_alpha_01;
layout(location = 2) out vec2 vary_alpha_23;
layout(location = 3) out vec2 vary_alpha_final;
layout(location = 4) out vec3 vary_normal;
layout(location = 5) out vec3 vary_position;
layout(location = 6) out vec2 vary_detail_texcoord1;
layout(location = 7) out vec2 vary_detail_texcoord2;
layout(location = 8) out vec2 vary_detail_texcoord3;
layout(location = 9) out vec2 vary_paint_texcoord;
layout(location = 10) out vec4 vary_terrain_tangent0;
layout(location = 11) out vec4 vary_terrain_tangent1;
layout(location = 12) out vec4 vary_terrain_tangent2;
layout(location = 13) out vec4 vary_terrain_tangent3;
layout(location = 14) out vec3 vary_lighting_normal;

vec2 khr_texture_transform(vec2 texcoord, vec2 scale, float rotation, vec2 offset)
{
    mat3 scale_mat = mat3(scale.x, 0, 0, 0, scale.y, 0, 0, 0, 1);
    mat3 offset_mat = mat3(1, 0, 0, 0, 1, 0, offset.x, offset.y, 1);
    mat3 rotation_mat = mat3(
        cos(rotation), -sin(rotation), 0,
        sin(rotation),  cos(rotation), 0,
        0,              0,             1);

    mat3 transform = offset_mat * rotation_mat * scale_mat;
    return (transform * vec3(texcoord, 1)).xy;
}

vec2 terrain_texture_transform(vec2 vertex_texcoord, vec3 transform0, vec2 transform1)
{
    vec2 texcoord = vertex_texcoord;

    texcoord.y = -texcoord.y;
    texcoord = khr_texture_transform(texcoord, transform0.xy, transform0.z, transform1);
    texcoord.y = -texcoord.y;

    return texcoord;
}

void terrain_material_transform(int material, out vec3 transform0, out vec2 transform1)
{
    if (material == 0)
    {
        transform0 = pc.terrain_texture_transform0.xyz;
        transform1 = vec2(pc.terrain_texture_transform0.w, pc.terrain_texture_transform1.x);
    }
    else if (material == 1)
    {
        transform0 = pc.terrain_texture_transform1.yzw;
        transform1 = pc.terrain_texture_transform2.xy;
    }
    else if (material == 2)
    {
        transform0 = vec3(
            pc.terrain_texture_transform2.zw,
            pc.terrain_texture_transform3.x);
        transform1 = pc.terrain_texture_transform3.yz;
    }
    else
    {
        transform0 = vec3(
            pc.terrain_texture_transform3.w,
            pc.terrain_texture_transform4.xy);
        transform1 = pc.terrain_texture_transform4.zw;
    }
}

vec2 terrain_detail_texcoord(int material)
{
    vec3 transform0;
    vec2 transform1;
    terrain_material_transform(material, transform0, transform1);
    return terrain_texture_transform(position.xy, transform0, transform1);
}

vec4 terrain_tangent_space_transform(vec4 vertex_tangent, vec3 vertex_normal, vec3 transform0)
{
    vec2 weights = vec2(1.0, 0.0);
    float khr_rotation = -transform0.z;
    mat2 khr_rotation_mat = mat2(
        cos(khr_rotation), -sin(khr_rotation),
        sin(khr_rotation),  cos(khr_rotation));
    weights = khr_rotation_mat * weights;

    vec2 khr_scale_sign = sign(transform0.xy);
    khr_scale_sign.x += 1.0 - abs(sign(khr_scale_sign.x));
    khr_scale_sign.y += 1.0 - abs(sign(khr_scale_sign.y));
    weights *= khr_scale_sign.xy;
    weights.x += 1.0 - abs(sign(sign(weights.x) + (0.5 * sign(weights.y))));
    weights.y = -weights.y;

    vec3 vertex_binormal = vertex_tangent.w * cross(vertex_normal, vertex_tangent.xyz);
    float sign_flip = khr_scale_sign.x * khr_scale_sign.y;
    return vec4(
        normalize((weights.x * vertex_tangent.xyz) + (weights.y * vertex_binormal)),
        vertex_tangent.w * sign_flip);
}

vec4 terrain_material_tangent(int material, vec3 vertex_tangent, vec3 vertex_normal)
{
    vec3 transform0;
    vec2 transform1;
    terrain_material_transform(material, transform0, transform1);
    return terrain_tangent_space_transform(vec4(vertex_tangent, tangent.w), vertex_normal, transform0);
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
    gl_Position = pc.modelview_projection_matrix * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;
    vec3 lighting_normal =
        safe_normalize(mat3(pc.normal_matrix) * normal, vec3(0.0, 0.0, 1.0));
    vec3 lighting_tangent =
        safe_normalize(mat3(pc.normal_matrix) * tangent.xyz, vec3(1.0, 0.0, 0.0));

    vary_detail_texcoord0 = terrain_detail_texcoord(0);
    vary_detail_texcoord1 = terrain_detail_texcoord(1);
    vary_detail_texcoord2 = terrain_detail_texcoord(2);
    vary_detail_texcoord3 = terrain_detail_texcoord(3);
    vary_alpha_01 = texcoord1;
    vary_alpha_23 = texcoord1 - vec2(2.0, 0.0);
    vary_alpha_final = texcoord1 - vec2(1.0, 0.0);
    vary_normal = normal;
    vary_position = position;
    vary_paint_texcoord = position.xy / max(pc.terrain_params.x, 1.0);
    vary_terrain_tangent0 = terrain_material_tangent(0, lighting_tangent, lighting_normal);
    vary_terrain_tangent1 = terrain_material_tangent(1, lighting_tangent, lighting_normal);
    vary_terrain_tangent2 = terrain_material_tangent(2, lighting_tangent, lighting_normal);
    vary_terrain_tangent3 = terrain_material_tangent(3, lighting_tangent, lighting_normal);
    vary_lighting_normal = lighting_normal;
}
