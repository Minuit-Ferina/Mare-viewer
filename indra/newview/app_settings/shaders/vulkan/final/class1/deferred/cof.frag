#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D depthMap;

layout(set = 1, binding = 0) uniform MareCoFUniforms
{
    mat4 inv_proj;
    vec4 params0;
    vec4 params1;
} u;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

float focal_distance()
{
    return u.params0.z;
}

float blur_constant()
{
    return u.params0.w;
}

float tan_pixel_angle()
{
    return u.params1.x;
}

float magnification()
{
    return u.params1.y;
}

float max_cof()
{
    return u.params1.z;
}

float calc_cof(float depth)
{
    float sc = (depth - focal_distance()) / -depth * blur_constant();

    sc /= magnification();

    float pixel_length = tan_pixel_angle() * -focal_distance();

    sc = sc / pixel_length;
    sc *= 1.414;

    return sc;
}

void main()
{
    vec2 tc = vary_fragcoord.xy;

    float z = texture(depthMap, tc).r;
    z = z * 2.0 - 1.0;
    vec4 ndc = vec4(0.0, 0.0, z, 1.0);
    vec4 p = u.inv_proj * ndc;
    float depth = p.z / p.w;

    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);

    float sc = calc_cof(depth);
    sc = min(sc, max_cof());
    sc = max(sc, -max_cof());

    frag_color.rgb = diff.rgb;
    frag_color.a = sc / max_cof() * 0.5 + 0.5;
}
