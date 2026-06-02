#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D exposureMap;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
} pc;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

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

float exposure()
{
    return pc.params0.x;
}

float tonemap_mix()
{
    return pc.params0.y;
}

int tonemap_type()
{
    return int(pc.params0.z + 0.5);
}

float gamma_value()
{
    return pc.params0.w;
}

vec3 linear_to_srgb(vec3 color)
{
    color = clamp(color, vec3(0.0), vec3(1.0));
    vec3 low_range = color * 12.92;
    vec3 high_range = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    bvec3 use_low_range = lessThan(color, vec3(0.0031308));
    return mix(high_range, low_range, use_low_range);
}

vec3 legacyGamma(vec3 color)
{
    vec3 c = 1.0 - clamp(color, vec3(0.0), vec3(1.0));
    c = 1.0 - pow(c, vec3(gamma_value()));
    return c;
}

vec3 RRTAndODTFit(vec3 color)
{
    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    return a / b;
}

vec3 toneMapACES_Hill(vec3 color)
{
    color = ACESInputMat * color;
    color = RRTAndODTFit(color);
    color = ACESOutputMat * color;
    return clamp(color, 0.0, 1.0);
}

vec3 PBRNeutralToneMapping(vec3 color)
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

vec3 toneMap(vec3 color)
{
#ifndef NO_POST
    vec3 linear_input_color = color;

    float exp_scale = texture(exposureMap, vec2(0.5, 0.5)).r;
    float final_exposure = exposure() * exp_scale;
    vec3 exposed_color = color * final_exposure;

    vec3 tonemapped_color = exposed_color;
    switch (tonemap_type())
    {
        case 0:
            tonemapped_color = PBRNeutralToneMapping(exposed_color);
            break;
        case 1:
            tonemapped_color = toneMapACES_Hill(exposed_color);
            break;
    }

    vec3 exposed_linear_input = linear_input_color * final_exposure;
    color = mix(exposed_linear_input, tonemapped_color, tonemap_mix());
    color = clamp(color, 0.0, 1.0);
#else
    color *= exposure() * texture(exposureMap, vec2(0.5, 0.5)).r;
    color = clamp(color, 0.0, 1.0);
#endif

    return color;
}

void main()
{
    vec4 diff = texture(diffuseRect, vary_fragcoord);

#ifndef NO_POST
    diff.rgb = toneMap(diff.rgb);
#else
    diff.rgb = clamp(diff.rgb, vec3(0.0), vec3(1.0));
#endif

#ifdef GAMMA_CORRECT
    diff.rgb = linear_to_srgb(diff.rgb);

#ifdef LEGACY_GAMMA
    diff.rgb = legacyGamma(diff.rgb);
#endif

#endif

    diff.rgb = clamp(diff.rgb, vec3(0.0), vec3(1.0));
    frag_color = diff;
}
