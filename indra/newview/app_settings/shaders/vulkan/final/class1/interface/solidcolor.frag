#version 450

layout(set = 0, binding = 0) uniform sampler2D tex0;

layout(push_constant) uniform MareInterfacePushConstants
{
    layout(offset = 128) vec4 color;
} pc;

layout(location = 0) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    float alpha = texture(tex0, vary_texcoord0).a * pc.color.a;
    frag_color = max(vec4(pc.color.rgb, alpha), vec4(0.0));
}
