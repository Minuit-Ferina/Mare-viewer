#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 1) uniform sampler2D gbufferColorMap;
layout(set = 0, binding = 2) uniform sampler2D gbufferSpecularOrOrmMap;
layout(set = 0, binding = 3) uniform sampler2D gbufferNormalMap;
layout(set = 0, binding = 4) uniform sampler2D gbufferEmissiveMap;
layout(set = 0, binding = 5) uniform sampler2D depthMap;
layout(set = 0, binding = 6) uniform sampler2D exposureMap;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 final_params;
    layout(offset = 176) vec4 final_post_params;
    layout(offset = 192) vec4 final_legacy_params;
    layout(offset = 208) vec4 final_post_modes;
} pc;

const mat3 ACESInputMat = mat3
(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
);

const mat3 ACESOutputMat = mat3
(
    1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602
);

vec3 RRTAndODTFit(vec3 color)
{
    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    return a / b;
}

vec3 tonemap_aces_hill(vec3 color)
{
    color = ACESInputMat * color;
    color = RRTAndODTFit(color);
    color = ACESOutputMat * color;
    return clamp(color, 0.0, 1.0);
}

vec3 tonemap_pbr_neutral(vec3 color)
{
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression)
    {
        return color;
    }

    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return mix(color, newPeak * vec3(1.0), g);
}

vec3 tonemap_final(vec3 color, float tonemap_type)
{
    if (tonemap_type > 0.5)
    {
        return tonemap_aces_hill(color);
    }
    return tonemap_pbr_neutral(color);
}

vec3 linear_to_srgb(vec3 color)
{
    color = clamp(color, vec3(0.0), vec3(1.0));
    bvec3 cutoff = lessThan(color, vec3(0.0031308));
    vec3 low = color * 12.92;
    vec3 high = 1.055 * pow(color, vec3(0.41666)) - 0.055;
    return mix(high, low, cutoff);
}

vec3 legacy_gamma(vec3 color, float gamma)
{
    vec3 inverted = 1.0 - clamp(color, vec3(0.0), vec3(1.0));
    return 1.0 - pow(inverted, vec3(gamma));
}

vec3 apply_final_transform(vec3 color, float exposure, float legacy_gamma_exponent, float gamma_mode, float tonemap_mix, float tonemap_type)
{
    float exposure_scale = max(texture(exposureMap, vec2(0.5, 0.5)).r, 0.0);
    vec3 rgb = max(color * exposure * exposure_scale, vec3(0.0));
    rgb = mix(rgb, tonemap_final(rgb, tonemap_type), tonemap_mix);
    if (gamma_mode > 0.5)
    {
        rgb = linear_to_srgb(rgb);
        if (gamma_mode > 1.5)
        {
            rgb = legacy_gamma(rgb, legacy_gamma_exponent);
        }
    }
    return rgb;
}

float extract_glow_weight(vec4 color, float max_extract_alpha, float warmth_amount)
{
    float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    float warmth = max(color.r, max(color.g * 0.65, color.b * 0.35));
    float overbright = smoothstep(1.0, 2.0, mix(luma, warmth, warmth_amount));
    return clamp(max(color.a, overbright * max_extract_alpha), 0.0, 1.0);
}

float luminance(vec3 color)
{
    return dot(color, vec3(0.299, 0.587, 0.114));
}

vec3 visualize_gbuffer(float mode, float attachment_count, vec2 texcoord)
{
    if (mode < 0.5)
    {
        return texture(gbufferColorMap, texcoord).rgb;
    }
    if (mode < 1.5 && attachment_count > 1.5)
    {
        return texture(gbufferSpecularOrOrmMap, texcoord).rgb;
    }
    if (mode < 2.5 && attachment_count > 2.5)
    {
        vec4 encoded_normal = texture(gbufferNormalMap, texcoord);
        return encoded_normal.xyz;
    }
    if (mode < 3.5 && attachment_count > 3.5)
    {
        return texture(gbufferEmissiveMap, texcoord).rgb;
    }
    if (mode < 4.5)
    {
        return vec3(luminance(texture(diffuseMap, texcoord).rgb));
    }
    if (mode < 5.5)
    {
        vec2 texel = 1.0 / max(vec2(textureSize(diffuseMap, 0)), vec2(1.0));
        float left = luminance(texture(diffuseMap, texcoord - vec2(texel.x, 0.0)).rgb);
        float right = luminance(texture(diffuseMap, texcoord + vec2(texel.x, 0.0)).rgb);
        float down = luminance(texture(diffuseMap, texcoord - vec2(0.0, texel.y)).rgb);
        float up = luminance(texture(diffuseMap, texcoord + vec2(0.0, texel.y)).rgb);
        float edge = max(abs(left - right), abs(down - up));
        return vec3(clamp(edge * 8.0, 0.0, 1.0));
    }
    if (mode < 6.5)
    {
        vec2 texel = 1.0 / max(vec2(textureSize(diffuseMap, 0)), vec2(1.0));
        float center = luminance(texture(diffuseMap, texcoord).rgb);
        float horizontal =
            abs(center - luminance(texture(diffuseMap, texcoord - vec2(texel.x, 0.0)).rgb)) +
            abs(center - luminance(texture(diffuseMap, texcoord + vec2(texel.x, 0.0)).rgb));
        float vertical =
            abs(center - luminance(texture(diffuseMap, texcoord - vec2(0.0, texel.y)).rgb)) +
            abs(center - luminance(texture(diffuseMap, texcoord + vec2(0.0, texel.y)).rgb));
        return vec3(
            clamp(horizontal * 4.0, 0.0, 1.0),
            clamp(vertical * 4.0, 0.0, 1.0),
            clamp(abs(horizontal - vertical) * 4.0, 0.0, 1.0));
    }
    return texture(gbufferColorMap, texcoord).rgb;
}

void main()
{
    vec4 color = texture(diffuseMap, vary_texcoord0.xy) * vertex_color;
    float exposure = max(pc.final_params.x, 0.0);
    float legacy_gamma_exponent = max(pc.final_params.y, 0.0001);
    float gamma_mode = pc.final_params.z;
    float tonemap_mix = clamp(pc.final_post_params.x, 0.0, 1.0);
    float tonemap_type = pc.final_post_params.y;
    float glow_warmth_amount = clamp(pc.final_post_params.z, 0.0, 1.0);
    float cas_sharpness = clamp(pc.final_params.w, 0.0, 1.0);
    float fsaa_type = pc.final_legacy_params.x;
    float buffer_visualization = pc.final_legacy_params.y;
    float gbuffer_attachment_count = pc.final_legacy_params.z;
    float dof_strength = max(pc.final_legacy_params.w, 0.0);
    float glow_strength = max(pc.final_post_modes.x, 0.0);
    float glow_max_extract_alpha = clamp(pc.final_post_modes.y, 0.0, 1.0);
    float glow_width = max(pc.final_post_modes.z, 0.0);
    float glow_iterations = max(pc.final_post_modes.w, 0.0);

    if (buffer_visualization >= 0.0)
    {
        frag_color = vec4(visualize_gbuffer(buffer_visualization, gbuffer_attachment_count, vary_texcoord0.xy), 1.0);
        return;
    }

    vec3 rgb = apply_final_transform(color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
    if (glow_strength > 0.001)
    {
        vec2 texel = 1.0 / max(vec2(textureSize(diffuseMap, 0)), vec2(1.0));
        vec2 glow_texel = texel * max(1.0, glow_width) * max(1.0, glow_iterations * 0.5);
        vec3 glow_accum = vec3(0.0);
        const float kernel_sum = 5.10;
        float kernel[8] = float[8](0.25, 0.5, 0.8, 1.0, 1.0, 0.8, 0.5, 0.25);
        float offsets[8] = float[8](-3.5, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5, 3.5);
        for (int i = 0; i < 8; ++i)
        {
            vec4 sample_raw = texture(diffuseMap, vary_texcoord0.xy + vec2(glow_texel.x * offsets[i], 0.0)) * vertex_color;
            glow_accum += kernel[i] *
                apply_final_transform(sample_raw.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type) *
                extract_glow_weight(sample_raw, glow_max_extract_alpha, glow_warmth_amount);

            sample_raw = texture(diffuseMap, vary_texcoord0.xy + vec2(0.0, glow_texel.y * offsets[i])) * vertex_color;
            glow_accum += kernel[i] *
                apply_final_transform(sample_raw.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type) *
                extract_glow_weight(sample_raw, glow_max_extract_alpha, glow_warmth_amount);
        }
        rgb = clamp(rgb + glow_accum * glow_strength * (0.5 / kernel_sum), vec3(0.0), vec3(1.0));
    }
    if (dof_strength > 0.001)
    {
        vec2 texel = 1.0 / max(vec2(textureSize(diffuseMap, 0)), vec2(1.0));
        float center_depth = texture(depthMap, vec2(0.5, 0.5)).r;
        float pixel_depth = texture(depthMap, vary_texcoord0.xy).r;
        float cof = clamp(abs(pixel_depth - center_depth) * dof_strength * 8.0, 0.0, 1.0);
        vec2 dof_texel = texel * mix(1.0, 4.0, cof);
        vec3 blur =
            apply_final_transform(texture(diffuseMap, vary_texcoord0.xy + vec2(dof_texel.x, 0.0)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type) +
            apply_final_transform(texture(diffuseMap, vary_texcoord0.xy - vec2(dof_texel.x, 0.0)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type) +
            apply_final_transform(texture(diffuseMap, vary_texcoord0.xy + vec2(0.0, dof_texel.y)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type) +
            apply_final_transform(texture(diffuseMap, vary_texcoord0.xy - vec2(0.0, dof_texel.y)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        rgb = mix(rgb, blur * 0.25, cof);
    }
    if (cas_sharpness > 0.001)
    {
        vec2 texel = 1.0 / max(vec2(textureSize(diffuseMap, 0)), vec2(1.0));
        vec3 left = apply_final_transform(texture(diffuseMap, vary_texcoord0.xy - vec2(texel.x, 0.0)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        vec3 right = apply_final_transform(texture(diffuseMap, vary_texcoord0.xy + vec2(texel.x, 0.0)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        vec3 down = apply_final_transform(texture(diffuseMap, vary_texcoord0.xy - vec2(0.0, texel.y)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        vec3 up = apply_final_transform(texture(diffuseMap, vary_texcoord0.xy + vec2(0.0, texel.y)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        vec3 blur = (left + right + down + up) * 0.25;
        rgb = clamp(rgb + (rgb - blur) * cas_sharpness * 0.65, vec3(0.0), vec3(1.0));
    }
    if (fsaa_type > 0.5)
    {
        vec2 texel = 1.0 / max(vec2(textureSize(diffuseMap, 0)), vec2(1.0));
        vec3 left = apply_final_transform(texture(diffuseMap, vary_texcoord0.xy - vec2(texel.x, 0.0)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        vec3 right = apply_final_transform(texture(diffuseMap, vary_texcoord0.xy + vec2(texel.x, 0.0)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        vec3 down = apply_final_transform(texture(diffuseMap, vary_texcoord0.xy - vec2(0.0, texel.y)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        vec3 up = apply_final_transform(texture(diffuseMap, vary_texcoord0.xy + vec2(0.0, texel.y)).rgb * vertex_color.rgb, exposure, legacy_gamma_exponent, gamma_mode, tonemap_mix, tonemap_type);
        float edge =
            max(abs(luminance(left) - luminance(right)), abs(luminance(down) - luminance(up)));
        float blend = clamp((edge - 0.03125) * 8.0, 0.0, 1.0);
        blend *= fsaa_type > 1.5 ? 0.65 : 0.45;
        rgb = mix(rgb, (left + right + down + up) * 0.25, blend);
    }

    frag_color = vec4(rgb, 1.0);
}
