#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
} pc;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

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
    c = 1.0 - pow(c, vec3(pc.params0.x));
    return c;
}

void main()
{
    vec4 color = texture(diffuseRect, vary_fragcoord);
    color.rgb = linear_to_srgb(color.rgb);
#ifdef LEGACY_GAMMA
    color.rgb = legacyGamma(color.rgb);
#endif
    color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
    frag_color = color;
}
