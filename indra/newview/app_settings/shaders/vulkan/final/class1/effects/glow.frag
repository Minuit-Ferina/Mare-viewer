#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
} pc;

layout(location = 0) in vec4 vary_texcoord0;
layout(location = 1) in vec4 vary_texcoord1;
layout(location = 2) in vec4 vary_texcoord2;
layout(location = 3) in vec4 vary_texcoord3;

layout(location = 0) out vec4 frag_color;

void main()
{
    float kernel[8] = float[8](0.25, 0.5, 0.8, 1.0, 1.0, 0.8, 0.5, 0.25);
    vec4 color = vec4(0.0);

    color += kernel[0] * texture(diffuseMap, vary_texcoord0.xy);
    color += kernel[1] * texture(diffuseMap, vary_texcoord1.xy);
    color += kernel[2] * texture(diffuseMap, vary_texcoord2.xy);
    color += kernel[3] * texture(diffuseMap, vary_texcoord3.xy);
    color += kernel[4] * texture(diffuseMap, vary_texcoord0.zw);
    color += kernel[5] * texture(diffuseMap, vary_texcoord1.zw);
    color += kernel[6] * texture(diffuseMap, vary_texcoord2.zw);
    color += kernel[7] * texture(diffuseMap, vary_texcoord3.zw);

    frag_color = max(vec4(color.rgb * pc.params0.z, color.a), vec4(0.0));
}
