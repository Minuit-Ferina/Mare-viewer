#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D emissiveRect;
layout(set = 0, binding = 2) uniform sampler2D normalMap;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
} pc;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

const float GBUFFER_FLAG_SKIP_ATMOS = 0.0;
const float GBUFFER_FLAG_HAS_HDRI = 1.0;

bool get_gbuffer_flag(float data, float flag)
{
    return abs(data - flag) < 0.1;
}

float luminance(vec3 color)
{
    return dot(vec3(0.2126, 0.7152, 0.0722), color);
}

void main()
{
    vec2 texcoord = vary_fragcoord * 0.6 + 0.2;
    texcoord.y -= 0.1;

    vec3 color = texture(diffuseRect, texcoord).rgb;
    vec4 normal = texture(normalMap, texcoord);

    if (!get_gbuffer_flag(normal.w, GBUFFER_FLAG_HAS_HDRI) &&
        !get_gbuffer_flag(normal.w, GBUFFER_FLAG_SKIP_ATMOS))
    {
        color *= pc.params0.x;
    }

    color += texture(emissiveRect, texcoord).rgb;
    frag_color = vec4(max(luminance(color), 0.0));
}
