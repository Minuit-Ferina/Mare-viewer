// Vulkan final shader source port.
// Source OpenGL shader: class1/deferred/treeShadowF.glsl

#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 64) vec4 params;
} pc;

layout(location = 1) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    float alpha = texture(diffuseMap, vary_texcoord0.xy).a;

    if (alpha < pc.params.x)
    {
        discard;
    }

    frag_color = vec4(1.0);
}
