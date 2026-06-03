#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D tex2;
layout(set = 0, binding = 3) uniform sampler2D tex3;
layout(set = 0, binding = 4) uniform sampler2D tex4;
layout(set = 0, binding = 5) uniform sampler2D tex5;
layout(set = 0, binding = 6) uniform sampler2D tex6;
layout(set = 0, binding = 7) uniform sampler2D tex7;
layout(set = 0, binding = 8) uniform sampler2D sceneDepthMap;
layout(set = 0, binding = 9) uniform sampler2D sceneColorMap;

layout(std140, set = 1, binding = 0) uniform DeferredSoften
{
    mat4 inverse_modelview_delta;
    vec4 atmos_blue_horizon_haze;
    vec4 atmos_blue_density_haze;
    vec4 atmos_density;
    vec4 atmos_glow;
    vec4 atmos_sunlight;
    vec4 atmos_moonlight;
    vec4 atmos_ambient;
    vec4 atmos_lightnorm;
    vec4 ssao_effect_mat0;
    vec4 ssao_effect_mat1;
    vec4 ssao_effect_mat2;
} soften;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 64) vec4 params;
    layout(offset = 80) vec4 material_params;
    layout(offset = 96) vec4 composite_clip_plane;
    layout(offset = 112) vec4 composite_sun_direction;
    layout(offset = 128) vec4 material_extra;
    layout(offset = 144) vec4 composite_moon_direction;
    layout(offset = 160) vec4 composite_sky_settings;
    layout(offset = 176) vec4 material_pbr;
    layout(offset = 192) vec4 material_legacy;
    layout(offset = 208) vec4 material_modes;
    layout(offset = 224) vec4 material_texture_transform2;
    layout(offset = 240) vec4 material_texture_transform3;
    layout(offset = 256) vec4 material_texture_transform4;
    layout(offset = 352) mat4 inverse_projection;
    layout(offset = 416) vec4 scene_ambient_direct_scale;
    layout(offset = 432) vec4 scene_direct_color;
    layout(offset = 448) vec4 scene_light_direction;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 2) flat in uint vary_texture_index;

layout(location = 0) out vec4 frag_color;

const uint MATERIAL_ATMOSPHERIC_HAZE = 8192u;
const uint MATERIAL_WATER_HAZE = 16384u;
const uint MATERIAL_WATER_EXCLUSION_MASK = 32768u;
const uint MATERIAL_HAS_SCENE_DEPTH = 131072u;
const uint MATERIAL_HAS_SCENE_COLOR = 262144u;

bool has_material_flag(uint flag)
{
    return (uint(pc.material_pbr.z + 0.5) & flag) != 0u;
}

vec3 srgb_to_linear(vec3 color)
{
    bvec3 cutoff = lessThanEqual(color, vec3(0.04045));
    vec3 low = color / 12.92;
    vec3 high = pow((color + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
}

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

vec3 safe_normalize(vec3 value)
{
    float len2 = dot(value, value);
    if (len2 <= 0.000001)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    return value * inversesqrt(len2);
}

float ambient_lighting(vec3 norm, vec3 light_dir)
{
    float ambient = min(abs(dot(norm.xyz, light_dir.xyz)), 1.0);
    ambient *= 0.5;
    ambient *= ambient;
    return 1.0 - ambient;
}

vec3 reconstruct_view_position(vec2 texcoord, float depth)
{
    vec2 ndc_xy = texcoord * 2.0 - 1.0;
    vec4 ndc = vec4(ndc_xy, depth * 2.0 - 1.0, 1.0);
    vec4 position = pc.inverse_projection * ndc;
    position.xyz /= max(abs(position.w), 0.000001);
    return position.xyz;
}

void calc_atmospheric_vars(
    vec3 in_position_eye,
    vec3 light_dir,
    out vec3 sunlit,
    out vec3 amblit,
    out vec3 additive,
    out vec3 atten)
{
    vec3 rel_pos = in_position_eye;
    float max_y = max(soften.atmos_density.w, 0.000001);
    if (abs(rel_pos.y) > max_y)
    {
        rel_pos *= max_y / rel_pos.y;
    }

    vec3 rel_pos_norm = safe_normalize(rel_pos);
    float rel_pos_len = length(rel_pos);
    vec3 lightnorm = soften.atmos_lightnorm.xyz;
    if (dot(lightnorm, lightnorm) <= 0.000001)
    {
        lightnorm = light_dir;
    }

    vec3 sunlight =
        pc.composite_sun_direction.w > 0.5 ?
            soften.atmos_sunlight.rgb :
            soften.atmos_moonlight.rgb;
    vec3 blue_horizon = soften.atmos_blue_horizon_haze.rgb;
    vec3 blue_density = soften.atmos_blue_density_haze.rgb;
    float haze_horizon = soften.atmos_blue_horizon_haze.a;
    float haze_density = soften.atmos_blue_density_haze.a;
    float cloud_shadow = clamp(soften.atmos_density.x, 0.0, 1.0);
    float density_multiplier = max(soften.atmos_density.y, 0.0);
    float distance_multiplier = max(soften.atmos_density.z, 0.0);

    vec3 light_atten =
        (blue_density + vec3(haze_density * 0.25)) *
        (density_multiplier * max_y);
    vec3 combined_haze = max(blue_density + vec3(haze_density), vec3(0.000001));
    vec3 blue_weight = blue_density / combined_haze;
    vec3 haze_weight = vec3(haze_density) / combined_haze;

    float above_horizon_factor = 1.0 / max(0.000001, lightnorm.y);
    sunlight *= exp(-light_atten * above_horizon_factor);

    float density_dist = rel_pos_len * density_multiplier;
    combined_haze =
        exp(-combined_haze * density_dist * distance_multiplier);
    atten = combined_haze.rgb;

    float haze_glow = dot(rel_pos_norm, lightnorm.xyz);
    haze_glow *= max(0.0, dot(light_dir, rel_pos_norm));
    haze_glow = 1.0 - haze_glow;
    haze_glow = max(haze_glow, 0.001);
    haze_glow *= soften.atmos_glow.x;
    haze_glow =
        clamp(pow(haze_glow, soften.atmos_glow.z), -100000.0, 100000.0);
    haze_glow += 0.25;
    haze_glow *= soften.atmos_glow.w;

    vec3 ambient_color = soften.atmos_ambient.rgb;
    vec3 tmp_ambient =
        ambient_color + (vec3(1.0) - ambient_color) * cloud_shadow * 0.5;
    vec3 cs = sunlight.rgb * (1.0 - cloud_shadow);
    additive =
        (blue_horizon.rgb * blue_weight.rgb) * (cs + tmp_ambient.rgb) +
        (haze_horizon * haze_weight.rgb) *
            (cs * haze_glow + tmp_ambient.rgb);

    sunlit = sunlight.rgb;
    amblit = pow(tmp_ambient.rgb, vec3(0.9)) * 0.57;
    additive *= vec3(1.0 - combined_haze);
    additive = min(additive, vec3(10.0));
}

void calc_atmospheric_vars_linear(
    vec3 in_position_eye,
    vec3 norm,
    vec3 light_dir,
    out vec3 sunlit,
    out vec3 amblit,
    out vec3 additive,
    out vec3 atten)
{
    calc_atmospheric_vars(
        in_position_eye,
        light_dir,
        sunlit,
        amblit,
        additive,
        atten);

    sunlit *= ambient_lighting(norm, light_dir);
    amblit *= ambient_lighting(norm, light_dir);
    sunlit *= soften.atmos_sunlight.a;
    amblit *= soften.atmos_moonlight.a;
}

vec4 diffuse_lookup(vec2 texcoord)
{
    if (pc.params.y < 0.5)
    {
        return texture(tex0, texcoord);
    }

    switch (vary_texture_index)
    {
        case 0u: return texture(tex0, texcoord);
        case 1u: return texture(tex1, texcoord);
        case 2u: return texture(tex2, texcoord);
        case 3u: return texture(tex3, texcoord);
        case 4u: return texture(tex4, texcoord);
        case 5u: return texture(tex5, texcoord);
        case 6u: return texture(tex6, texcoord);
        case 7u: return texture(tex7, texcoord);
        default: return texture(tex0, texcoord);
    }
}

vec2 screen_texcoord_from_depth()
{
    return clamp(vary_texcoord0.xy, vec2(0.0), vec2(1.0));
}

float depth_haze_opacity(vec2 screen_texcoord)
{
    if (!has_material_flag(MATERIAL_HAS_SCENE_DEPTH))
    {
        return 0.0;
    }

    float depth = texture(sceneDepthMap, screen_texcoord).r;
    if (depth >= 0.99999)
    {
        return 0.0;
    }
    return smoothstep(0.55, 0.985, depth);
}

vec4 apply_atmospheric_or_water_haze(vec4 color)
{
    vec2 screen_texcoord = screen_texcoord_from_depth();
    float opacity = depth_haze_opacity(screen_texcoord) * clamp(color.a, 0.0, 1.0);
    vec3 ambient = get_scene_ambient_color();
    vec3 direct = get_scene_direct_color();
    vec3 fog_color = srgb_to_linear(clamp(color.rgb, vec3(0.0), vec3(1.0)));
    fog_color *= clamp(ambient + direct * 0.28, vec3(0.05), vec3(2.0));
    float fog_mix = 0.42;

    if (has_material_flag(MATERIAL_WATER_HAZE))
    {
        float exclusion = texture(tex5, screen_texcoord).r;
        opacity *= mix(1.0, exclusion, 0.65);
        fog_color *= mix(vec3(0.72, 0.94, 1.08), direct + ambient, 0.22);
        fog_mix = 0.62;
    }
    else
    {
        fog_color *= mix(vec3(1.0), direct + ambient, 0.12);
    }

    float haze_contribution = opacity * fog_mix;
    color.rgb = fog_color * haze_contribution;
    color.a = 1.0 - haze_contribution;
    return color;
}

vec4 apply_atmospheric_haze()
{
    vec2 screen_texcoord = screen_texcoord_from_depth();
    float depth = texture(sceneDepthMap, screen_texcoord).r;
    if (!has_material_flag(MATERIAL_HAS_SCENE_DEPTH) || depth >= 0.99999)
    {
        discard;
    }

    vec3 position = reconstruct_view_position(screen_texcoord, depth);
    vec3 normal = vec3(0.0, 0.0, 1.0);
    vec3 light_dir =
        pc.composite_sun_direction.w > 0.5 ?
            pc.composite_sun_direction.xyz :
            pc.composite_moon_direction.xyz;
    if (dot(light_dir, light_dir) <= 0.000001)
    {
        light_dir = pc.scene_light_direction.xyz;
    }
    light_dir = safe_normalize(light_dir);

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calc_atmospheric_vars_linear(
        position,
        normal,
        light_dir,
        sunlit,
        amblit,
        additive,
        atten);

    bool do_atmospherics =
        pc.composite_clip_plane.w > 0.0 ||
        dot(position, pc.composite_clip_plane.xyz) +
            pc.composite_clip_plane.w > 0.0;

    if (!do_atmospherics)
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    vec3 color = srgb_to_linear(additive * 2.0);
    color *= max(pc.composite_sky_settings.y, 0.0);
    return max(vec4(color, atten.r), vec4(0.0));
}

void main()
{
    vec4 color =
        diffuse_lookup(vary_texcoord0.xy) *
        vertex_color *
        vec4(pc.material_params.rgb, pc.material_pbr.w);

    if (has_material_flag(MATERIAL_WATER_EXCLUSION_MASK))
    {
        frag_color = vec4(color.rgb, 1.0);
        return;
    }

    if (has_material_flag(MATERIAL_ATMOSPHERIC_HAZE))
    {
        frag_color = apply_atmospheric_haze();
        return;
    }

    if (has_material_flag(MATERIAL_WATER_HAZE))
    {
        frag_color = apply_atmospheric_or_water_haze(color);
        return;
    }

    frag_color = color;
}
