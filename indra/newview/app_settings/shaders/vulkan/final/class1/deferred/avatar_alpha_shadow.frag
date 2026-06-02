// Vulkan final shader source port.
// Source OpenGL shader: class1/deferred/avatarAlphaShadowF.glsl

#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform MareGeneratedFragmentConstants
{
    float minimum_alpha;
    vec3 _pad0;
    vec4 color;
    vec4 params;
} pc;

layout(location = 1) in vec2 vary_texcoord0;
layout(location = 8) in vec4 vertex_position;

layout(location = 0) out vec4 frag_color;

void main()
{
    float alpha = texture(diffuseMap, vary_texcoord0.xy).a * pc.color.a;

    if (alpha < 0.05)
    {
        discard;
    }

    if (alpha < pc.minimum_alpha &&
        fract(0.5 * floor(vertex_position.x / vertex_position.w)) < 0.25)
    {
        discard;
    }

    frag_color = vec4(1.0);
}
