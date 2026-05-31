#version 450

layout(set = 0, binding = 0) uniform sampler2D detail0;
layout(set = 0, binding = 1) uniform sampler2D detail1;
layout(set = 0, binding = 2) uniform sampler2D detail2;
layout(set = 0, binding = 3) uniform sampler2D detail3;
layout(set = 0, binding = 4) uniform sampler2D alphaRamp;
layout(set = 0, binding = 9) uniform sampler2D emissive0;
layout(set = 0, binding = 10) uniform sampler2D emissive1;
layout(set = 0, binding = 11) uniform sampler2D emissive2;
layout(set = 0, binding = 12) uniform sampler2D emissive3;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 terrain_params;
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
    layout(offset = 416) vec4 scene_ambient_direct_scale;
    layout(offset = 432) vec4 scene_direct_color;
    layout(offset = 448) vec4 scene_light_direction;
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

layout(location = 0) out vec4 frag_color;

const float TERRAIN_TRIPLANAR_MIX_THRESHOLD = 0.01;

vec3 get_scene_ambient_color()
{
    if (pc.scene_direct_color.a < 0.5)
    {
        return vec3(0.36);
    }
    return max(pc.scene_ambient_direct_scale.rgb, vec3(0.0));
}

vec3 get_scene_direct_color()
{
    if (pc.scene_direct_color.a < 0.5)
    {
        return vec3(1.0);
    }
    return max(pc.scene_direct_color.rgb, vec3(0.0)) *
        max(pc.scene_ambient_direct_scale.a, 0.0);
}

vec3 get_scene_light_direction()
{
    vec3 light_dir = pc.scene_light_direction.xyz;
    if (pc.scene_light_direction.w < 0.5 ||
        dot(light_dir, light_dir) <= 0.0001)
    {
        light_dir = vec3(0.35, 0.45, 0.82);
    }
    return normalize(light_dir);
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
            terrain_sample_rgba(emissive0, 0, vary_detail_texcoord0).rgb *
            weights.x +
        pc.terrain_emissive_min_alpha1.rgb *
            terrain_sample_rgba(emissive1, 1, vary_detail_texcoord1).rgb *
            weights.y +
        pc.terrain_emissive_min_alpha2.rgb *
            terrain_sample_rgba(emissive2, 2, vary_detail_texcoord2).rgb *
            weights.z +
        pc.terrain_emissive_min_alpha3.rgb *
            terrain_sample_rgba(emissive3, 3, vary_detail_texcoord3).rgb *
            weights.w;
}

vec3 terrain_direct_lighting(vec3 base_color)
{
    vec3 normal = normalize(vary_normal);
    if (dot(normal, normal) <= 0.0001)
    {
        normal = vec3(0.0, 0.0, 1.0);
    }

    vec3 direct_color = get_scene_direct_color();
    float ndotl = max(dot(normal, get_scene_light_direction()), 0.0);
    return
        base_color * get_scene_ambient_color() +
        base_color * direct_color * ndotl;
}

void main()
{
    vec4 color0 = terrain_sample_rgba(detail0, 0, vary_detail_texcoord0) * pc.terrain_base_color0;
    vec4 color1 = terrain_sample_rgba(detail1, 1, vary_detail_texcoord1) * pc.terrain_base_color1;
    vec4 color2 = terrain_sample_rgba(detail2, 2, vary_detail_texcoord2) * pc.terrain_base_color2;
    vec4 color3 = terrain_sample_rgba(detail3, 3, vary_detail_texcoord3) * pc.terrain_base_color3;

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

    frag_color = vec4(
        max(terrain_direct_lighting(color.rgb) + terrain_emissive(weights), vec3(0.0)),
        1.0);
}
