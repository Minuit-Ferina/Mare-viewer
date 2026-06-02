#version 450

layout(set = 0, binding = 0) uniform sampler2D detail0;
layout(set = 0, binding = 1) uniform sampler2D detail1;
layout(set = 0, binding = 2) uniform sampler2D detail2;
layout(set = 0, binding = 3) uniform sampler2D detail3;
layout(set = 0, binding = 4) uniform sampler2D alphaRamp;
layout(set = 0, binding = 5) uniform sampler2D orm0;
layout(set = 0, binding = 6) uniform sampler2D orm1;
layout(set = 0, binding = 7) uniform sampler2D orm2;
layout(set = 0, binding = 8) uniform sampler2D orm3;
layout(set = 0, binding = 9) uniform sampler2D emissive0;
layout(set = 0, binding = 10) uniform sampler2D emissive1;
layout(set = 0, binding = 11) uniform sampler2D emissive2;
layout(set = 0, binding = 12) uniform sampler2D emissive3;
layout(set = 0, binding = 13) uniform sampler2D normal0;
layout(set = 0, binding = 14) uniform sampler2D normal1;
layout(set = 0, binding = 15) uniform sampler2D normal2;
layout(set = 0, binding = 16) uniform sampler2D normal3;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 64) vec4 params;
    layout(offset = 80) vec4 terrain_params;
    layout(offset = 96) vec4 terrain_metallic;
    layout(offset = 112) vec4 terrain_roughness;
    layout(offset = 128) vec4 terrain_base_color0;
    layout(offset = 144) vec4 terrain_base_color1;
    layout(offset = 160) vec4 terrain_base_color2;
    layout(offset = 176) vec4 terrain_base_color3;
    layout(offset = 192) vec4 terrain_emissive_min_alpha0;
    layout(offset = 208) vec4 terrain_emissive_min_alpha1;
    layout(offset = 224) vec4 terrain_emissive_min_alpha2;
    layout(offset = 240) vec4 terrain_emissive_min_alpha3;
    layout(offset = 272) vec4 terrain_texture_transform0;
    layout(offset = 288) vec4 terrain_texture_transform1;
    layout(offset = 304) vec4 terrain_texture_transform2;
    layout(offset = 320) vec4 terrain_texture_transform3;
    layout(offset = 336) vec4 terrain_texture_transform4;
} pc;

layout(location = 0) in vec2 vary_detail_texcoord0;
layout(location = 1) in vec2 vary_alpha_01;
layout(location = 2) in vec2 vary_alpha_23;
layout(location = 3) in vec2 vary_alpha_final;
layout(location = 4) in vec3 vary_normal;
layout(location = 5) in vec3 vary_position;
layout(location = 6) in vec2 vary_detail_texcoord1;
layout(location = 7) in vec2 vary_detail_texcoord2;
layout(location = 8) in vec2 vary_detail_texcoord3;
layout(location = 9) in vec2 vary_paint_texcoord;
layout(location = 10) in vec4 vary_terrain_tangent0;
layout(location = 11) in vec4 vary_terrain_tangent1;
layout(location = 12) in vec4 vary_terrain_tangent2;
layout(location = 13) in vec4 vary_terrain_tangent3;
layout(location = 14) in vec3 vary_lighting_normal;

layout(location = 0) out vec4 frag_diffuse;
layout(location = 1) out vec4 frag_specular_or_orm;
layout(location = 2) out vec4 frag_normal;
layout(location = 3) out vec4 frag_emissive;

const float TERRAIN_TRIPLANAR_MIX_THRESHOLD = 0.01;

bool terrain_uses_pbr_materials()
{
    return pc.params.y > 0.5;
}

vec3 srgb_to_linear(vec3 color)
{
    bvec3 cutoff = lessThanEqual(color, vec3(0.04045));
    vec3 low = color / 12.92;
    vec3 high = pow((color + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
}

vec4 encode_normal(vec3 n, float gbuffer_flag)
{
    vec3 encoded = normalize(n) * 0.5 + 0.5;
    return vec4(encoded.xyz, gbuffer_flag);
}

bool terrain_use_triplanar()
{
    return pc.terrain_params.y > 2.5;
}

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

vec3 terrain_triplanar_weights()
{
    vec3 n = abs(normalize(vary_normal));
    vec3 weights = pow(n, vec3(max(pc.terrain_params.z, 0.001)));
    weights /= max(weights.x + weights.y + weights.z, 0.0001);
    weights = max(vec3(0.0), weights - vec3(TERRAIN_TRIPLANAR_MIX_THRESHOLD));
    float total = weights.x + weights.y + weights.z;
    return total > 0.0 ? weights / total : vec3(0.0, 0.0, 1.0);
}

vec2 terrain_axis_texcoord(int material, int axis)
{
    vec3 transform0;
    vec2 transform1;
    terrain_material_transform(material, transform0, transform1);

    vec2 uv = vary_position.xy;
    if (axis == 0)
    {
        uv = vary_normal.x >= 0.0 ?
            vary_position.yz :
            vary_position.yz * vec2(-1.0, 1.0);
    }
    else if (axis == 1)
    {
        uv = vary_normal.y >= 0.0 ?
            vary_position.xz * vec2(-1.0, 1.0) :
            vary_position.xz;
    }

    return terrain_texture_transform(uv, transform0, transform1);
}

vec4 terrain_sample_rgba(sampler2D tex, int material, vec2 planar_texcoord)
{
    if (!terrain_use_triplanar())
    {
        return texture(tex, planar_texcoord);
    }

    vec3 weights = terrain_triplanar_weights();
    return
        texture(tex, terrain_axis_texcoord(material, 0)) * weights.x +
        texture(tex, terrain_axis_texcoord(material, 1)) * weights.y +
        texture(tex, terrain_axis_texcoord(material, 2)) * weights.z;
}

vec4 terrain_sample_base_color(sampler2D tex, int material, vec2 planar_texcoord)
{
    vec4 color = terrain_sample_rgba(tex, material, planar_texcoord);
    if (terrain_uses_pbr_materials())
    {
        color.rgb = srgb_to_linear(max(color.rgb, vec3(0.0)));
    }
    return color;
}

vec3 terrain_sample_emissive(sampler2D tex, int material, vec2 planar_texcoord)
{
    vec3 emissive = terrain_sample_rgba(tex, material, planar_texcoord).rgb;
    if (terrain_uses_pbr_materials())
    {
        emissive = srgb_to_linear(max(emissive, vec3(0.0)));
    }
    return emissive;
}

vec3 terrain_base_normal()
{
    vec3 base_normal = normalize(vary_lighting_normal);
    if (dot(base_normal, base_normal) <= 0.0001)
    {
        base_normal = vec3(0.0, 0.0, 1.0);
    }
    return base_normal;
}

vec3 terrain_normal_post_1(vec3 normal_sample, float sign_or_zero)
{
    float axis_sign = sign_or_zero;
    axis_sign = (2.0 * axis_sign) + 1.0;
    axis_sign /= abs(axis_sign);
    normal_sample.xy =
        (min(0.0, axis_sign) * normal_sample.xy) +
        (min(0.0, -axis_sign) * -normal_sample.xy);
    return normal_sample;
}

vec3 terrain_normal_post_x(vec3 normal_sample, float tangent_sign)
{
    vec3 v = terrain_normal_post_1(normal_sample, sign(vary_normal.x));
    v.xy = vec2(-v.y, v.x);
    v.xy *= tangent_sign;
    return v;
}

vec3 terrain_normal_post_y(vec3 normal_sample)
{
    vec3 v = terrain_normal_post_1(normal_sample, sign(vary_normal.y));
    v.xy = -v.xy;
    return v;
}

vec3 terrain_normal_post_z(vec3 normal_sample)
{
    return terrain_normal_post_1(normal_sample, sign(vary_normal.z));
}

vec4 terrain_tangent_info(int material)
{
    if (material == 0)
    {
        return vary_terrain_tangent0;
    }
    if (material == 1)
    {
        return vary_terrain_tangent1;
    }
    if (material == 2)
    {
        return vary_terrain_tangent2;
    }
    return vary_terrain_tangent3;
}

vec3 terrain_sample_normal_axis(sampler2D tex, int material, int axis, float tangent_sign)
{
    vec3 normal_sample =
        texture(tex, terrain_axis_texcoord(material, axis)).xyz * 2.0 - 1.0;
    if (axis == 0)
    {
        return terrain_normal_post_x(normal_sample, tangent_sign);
    }
    if (axis == 1)
    {
        return terrain_normal_post_y(normal_sample);
    }
    return terrain_normal_post_z(normal_sample);
}

vec3 terrain_sample_normal(sampler2D tex, int material, vec2 planar_texcoord, vec4 tangent_info)
{
    if (!terrain_use_triplanar())
    {
        return texture(tex, planar_texcoord).xyz * 2.0 - 1.0;
    }

    vec3 weights = terrain_triplanar_weights();
    vec3 normal_sample =
        terrain_sample_normal_axis(tex, material, 0, tangent_info.w) * weights.x +
        terrain_sample_normal_axis(tex, material, 1, tangent_info.w) * weights.y +
        terrain_sample_normal_axis(tex, material, 2, tangent_info.w) * weights.z;
    return dot(normal_sample, normal_sample) > 0.0001 ?
        normalize(normal_sample) :
        vec3(0.0, 0.0, 1.0);
}

vec3 terrain_mikktspace(vec3 normal_sample, vec4 tangent_info)
{
    vec3 base_normal = terrain_base_normal();
    vec3 tangent = normalize(tangent_info.xyz);
    if (dot(tangent, tangent) <= 0.0001)
    {
        return base_normal;
    }

    vec3 bitangent = tangent_info.w * cross(base_normal, tangent);
    return normalize(
        normal_sample.x * tangent +
        normal_sample.y * bitangent +
        normal_sample.z * base_normal);
}

vec3 terrain_material_normal(sampler2D tex, int material, vec2 planar_texcoord)
{
    vec4 tangent_info = terrain_tangent_info(material);
    vec3 normal_sample = terrain_sample_normal(tex, material, planar_texcoord, tangent_info);
    return terrain_mikktspace(normal_sample, tangent_info);
}

vec3 terrain_normal(vec4 weights)
{
    vec3 base_normal = terrain_base_normal();
    vec3 map_normal =
        terrain_material_normal(normal0, 0, vary_detail_texcoord0) * weights.x +
        terrain_material_normal(normal1, 1, vary_detail_texcoord1) * weights.y +
        terrain_material_normal(normal2, 2, vary_detail_texcoord2) * weights.z +
        terrain_material_normal(normal3, 3, vary_detail_texcoord3) * weights.w;
    if (dot(map_normal, map_normal) <= 0.0001)
    {
        return base_normal;
    }
    return normalize(map_normal);
}

vec4 terrain_weights(float alpha1, float alpha2, float alphaFinal)
{
    return vec4(
        alphaFinal * alpha1,
        alphaFinal * (1.0 - alpha1),
        (1.0 - alphaFinal) * alpha2,
        (1.0 - alphaFinal) * (1.0 - alpha2));
}

vec4 terrain_weights_from_paint_map(vec3 paint)
{
    paint = max(vec3(0.0), paint);
    paint /= max(1.0, paint.x + paint.y + paint.z);
    return vec4(1.0 - (paint.x + paint.y + paint.z), paint.xyz);
}

vec4 terrain_weights()
{
    if (pc.terrain_params.w > 0.5)
    {
        return terrain_weights_from_paint_map(texture(alphaRamp, vary_paint_texcoord).rgb);
    }

    float alpha1 = texture(alphaRamp, vary_alpha_01).a;
    float alpha2 = texture(alphaRamp, vary_alpha_23).a;
    float alphaFinal = texture(alphaRamp, vary_alpha_final).a;
    return terrain_weights(alpha1, alpha2, alphaFinal);
}

float terrain_minimum_alpha(vec4 weights)
{
    return dot(
        vec4(
            pc.terrain_emissive_min_alpha0.a,
            pc.terrain_emissive_min_alpha1.a,
            pc.terrain_emissive_min_alpha2.a,
            pc.terrain_emissive_min_alpha3.a),
        weights);
}

vec3 terrain_emissive(vec4 weights)
{
    return
        pc.terrain_emissive_min_alpha0.rgb *
            terrain_sample_emissive(emissive0, 0, vary_detail_texcoord0) *
            weights.x +
        pc.terrain_emissive_min_alpha1.rgb *
            terrain_sample_emissive(emissive1, 1, vary_detail_texcoord1) *
            weights.y +
        pc.terrain_emissive_min_alpha2.rgb *
            terrain_sample_emissive(emissive2, 2, vary_detail_texcoord2) *
            weights.z +
        pc.terrain_emissive_min_alpha3.rgb *
            terrain_sample_emissive(emissive3, 3, vary_detail_texcoord3) *
            weights.w;
}

void main()
{
    vec4 color0 = terrain_sample_base_color(detail0, 0, vary_detail_texcoord0) * pc.terrain_base_color0;
    vec4 color1 = terrain_sample_base_color(detail1, 1, vary_detail_texcoord1) * pc.terrain_base_color1;
    vec4 color2 = terrain_sample_base_color(detail2, 2, vary_detail_texcoord2) * pc.terrain_base_color2;
    vec4 color3 = terrain_sample_base_color(detail3, 3, vary_detail_texcoord3) * pc.terrain_base_color3;

    vec4 weights = terrain_weights();

    vec4 color =
        color0 * weights.x +
        color1 * weights.y +
        color2 * weights.z +
        color3 * weights.w;

    if (color.a < terrain_minimum_alpha(weights))
    {
        discard;
    }

    frag_diffuse = vec4(max(color.rgb, vec3(0.0)), 0.0);
    if (terrain_uses_pbr_materials())
    {
        vec4 orm_sample0 = terrain_sample_rgba(orm0, 0, vary_detail_texcoord0);
        vec4 orm_sample1 = terrain_sample_rgba(orm1, 1, vary_detail_texcoord1);
        vec4 orm_sample2 = terrain_sample_rgba(orm2, 2, vary_detail_texcoord2);
        vec4 orm_sample3 = terrain_sample_rgba(orm3, 3, vary_detail_texcoord3);
        float occlusion =
            orm_sample0.r * weights.x +
            orm_sample1.r * weights.y +
            orm_sample2.r * weights.z +
            orm_sample3.r * weights.w;
        float roughness = dot(
            pc.terrain_roughness *
                vec4(orm_sample0.g, orm_sample1.g, orm_sample2.g, orm_sample3.g),
            weights);
        float metallic = dot(
            pc.terrain_metallic *
                vec4(orm_sample0.b, orm_sample1.b, orm_sample2.b, orm_sample3.b),
            weights);
        frag_specular_or_orm =
            vec4(
                clamp(occlusion, 0.0, 1.0),
                clamp(roughness, 0.04, 1.0),
                clamp(metallic, 0.0, 1.0),
                1.0);
        frag_normal = encode_normal(terrain_normal(weights), 1.0);
        frag_emissive = vec4(max(terrain_emissive(weights), vec3(0.0)), 0.0);
    }
    else
    {
        frag_specular_or_orm = vec4(0.0, 0.0, 0.0, -1.0);
        frag_normal = encode_normal(terrain_base_normal(), 1.0);
        frag_emissive = vec4(0.0);
    }
}
