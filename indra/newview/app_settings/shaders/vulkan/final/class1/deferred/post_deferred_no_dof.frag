#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D depthMap;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

vec3 clamp_hdr_range(vec3 color)
{
    return max(color, vec3(0.0));
}

void main()
{
    vec4 color = texture(diffuseRect, vary_fragcoord);
    color.rgb = clamp_hdr_range(color.rgb);
    frag_color = color;
    gl_FragDepth = texture(depthMap, vary_fragcoord).r;
}
