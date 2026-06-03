#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform MareDiffuseMaskPushConstants
{
    float minimum_alpha;
} pc;

layout(location = 0) in vec3 vary_normal;
layout(location = 2) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_data[4];

const float GBUFFER_FLAG_HAS_ATMOS = 0.34;

vec4 encode_normal(vec3 n, float env, float gbuffer_flag)
{
    n = normalize(n);
    float f = sqrt(8.0 * n.z + 8.0);
    return vec4(n.xy / f + 0.5, env, gbuffer_flag);
}

void main()
{
    vec4 color = texture(diffuseMap, vary_texcoord0.xy);

    if (color.a < pc.minimum_alpha)
    {
        discard;
    }

    frag_data[0] = vec4(color.rgb, 0.0);
    frag_data[1] = vec4(0.0);
    frag_data[2] = encode_normal(vary_normal, 0.0, GBUFFER_FLAG_HAS_ATMOS);
    frag_data[3] = vec4(0.0);
}
