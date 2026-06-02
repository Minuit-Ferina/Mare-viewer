#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
} pc;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

void main()
{
    vec4 diff = textureLod(diffuseRect, vary_fragcoord.xy, pc.params0.x);

    frag_color = diff;
}
