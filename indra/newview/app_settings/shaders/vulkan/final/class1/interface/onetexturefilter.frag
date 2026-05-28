#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;

layout(push_constant) uniform MareInterfacePushConstants
{
    layout(offset = 160) vec4 params;
} pc;

layout(location = 0) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = texture(tex0, vary_texcoord0);

    if (frag_color.r + frag_color.g + frag_color.b < pc.params.x)
    {
        discard;
    }

    frag_color.a = 0.95;
}
