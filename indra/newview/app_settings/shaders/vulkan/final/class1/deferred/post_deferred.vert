#version 450

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
} pc;

layout(location = 0) in vec3 position;

layout(location = 0) out vec2 vary_fragcoord;
layout(location = 1) out vec2 vary_texcoord0;

void main()
{
    vec4 pos = vec4(position, 1.0);
    gl_Position = pos;

    vec2 texcoord = pos.xy * 0.5 + 0.5;
    vary_fragcoord = texcoord;
    vary_texcoord0 = texcoord * pc.params0.xy;
}
