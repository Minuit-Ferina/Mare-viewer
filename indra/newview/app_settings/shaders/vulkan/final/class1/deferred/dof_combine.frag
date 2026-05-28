#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D lightMap;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
    vec4 params1;
} pc;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

vec4 dof_sample(sampler2D source, vec2 texcoord)
{
    texcoord.x = min(texcoord.x, pc.params1.z);
    texcoord.y = min(texcoord.y, pc.params1.w);
    return texture(source, texcoord);
}

void main()
{
    float max_cof = pc.params0.x;
    float res_scale = pc.params0.y;
    vec2 screen_res = pc.params1.xy;

    vec4 dof = dof_sample(diffuseRect, vary_fragcoord * res_scale);
    vec4 color = texture(lightMap, vary_fragcoord);
    float amount = min(abs(color.a * 2.0 - 1.0) * max_cof * res_scale * res_scale, 1.0);

    if (amount > 0.25 && amount < 0.75)
    {
        float scale = amount / res_scale;
        vec4 average =
            texture(lightMap, vary_fragcoord + vec2(scale, scale) / screen_res) +
            texture(lightMap, vary_fragcoord + vec2(-scale, scale) / screen_res) +
            texture(lightMap, vary_fragcoord + vec2(scale, -scale) / screen_res) +
            texture(lightMap, vary_fragcoord + vec2(-scale, -scale) / screen_res);
        color = mix(color, average * 0.25, amount);
    }

    frag_color = mix(color, dof, amount);
}
