#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D dither_tex;

layout(push_constant) uniform MareInterfacePushConstants
{
    layout(offset = 160) vec4 params;
} pc;

layout(location = 0) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = abs(texture(tex0, vary_texcoord0) - texture(tex1, vary_texcoord0));

    vec2 dither_coord = vec2(
        vary_texcoord0.x * pc.params.y,
        vary_texcoord0.y * pc.params.z);
    vec4 dither_vec = texture(dither_tex, dither_coord);

    for (int i = 0; i < 3; ++i)
    {
        if (frag_color[i] < dither_vec[i] * pc.params.x)
        {
            frag_color[i] = 0.0;
        }
    }
}
