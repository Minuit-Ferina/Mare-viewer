#version 450

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
} pc;

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 vary_texcoord0;
layout(location = 1) out vec4 vary_texcoord1;
layout(location = 2) out vec4 vary_texcoord2;
layout(location = 3) out vec4 vary_texcoord3;

void main()
{
    gl_Position = vec4(position, 1.0);

    vec2 texcoord = position.xy * 0.5 + 0.5;
    vec2 glow_delta = pc.params0.xy;

    vary_texcoord0.xy = texcoord + glow_delta * -3.5;
    vary_texcoord1.xy = texcoord + glow_delta * -2.5;
    vary_texcoord2.xy = texcoord + glow_delta * -1.5;
    vary_texcoord3.xy = texcoord + glow_delta * -0.5;
    vary_texcoord0.zw = texcoord + glow_delta * 0.5;
    vary_texcoord1.zw = texcoord + glow_delta * 1.5;
    vary_texcoord2.zw = texcoord + glow_delta * 2.5;
    vary_texcoord3.zw = texcoord + glow_delta * 3.5;
}
