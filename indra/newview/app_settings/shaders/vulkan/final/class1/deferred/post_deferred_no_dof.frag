#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D depthMap;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
} pc;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

float hash(float n)
{
    return fract(sin(n) * 1e4);
}

float hash(vec2 p)
{
    return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) *
        (0.1 + abs(sin(p.y * 13.0 + p.x))));
}

float noise(float x)
{
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

float noise(vec2 x)
{
    vec2 i = floor(x);
    vec2 f = fract(x);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) +
        (c - a) * u.y * (1.0 - u.x) +
        (d - b) * u.x * u.y;
}

vec3 clampHDRRange(vec3 color)
{
    color = mix(color, vec3(1.0), isinf(color));
    color = mix(color, vec3(0.0), isnan(color));
    return clamp(color, vec3(0.0), vec3(11.2));
}

void main()
{
    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);

#ifdef HAS_NOISE
    vec2 tc = vary_fragcoord.xy * pc.params0.xy * 4.0;
    vec3 seed = (diff.rgb + vec3(1.0)) * vec3(tc.xy, tc.x + tc.y);
    vec3 nz = vec3(noise(seed.rg), noise(seed.gb), noise(seed.rb));
    diff.rgb += nz * 0.003;
#endif

    diff.rgb = clampHDRRange(diff.rgb);
    frag_color = diff;

    gl_FragDepth = texture(depthMap, vary_fragcoord.xy).r;
}
