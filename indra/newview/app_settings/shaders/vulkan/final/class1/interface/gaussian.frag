#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;

layout(push_constant) uniform MareInterfacePushConstants
{
    layout(offset = 160) vec4 params;
} pc;

layout(location = 0) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    vec3 color = vec3(0.0);
    float weights[9] = float[9](
        0.0002,
        0.0060,
        0.0606,
        0.2417,
        0.3829,
        0.2417,
        0.0606,
        0.0060,
        0.0002);

    vec2 direction = pc.params.yz;
    float res_scale = pc.params.x;
    for (int i = 0; i < 9; ++i)
    {
        vec2 texcoord = vary_texcoord0 + float(i - 4) * direction * res_scale;
        color += texture(diffuseRect, texcoord).rgb * weights[i];
    }

    frag_color = max(vec4(color, 0.0), vec4(0.0));
}
