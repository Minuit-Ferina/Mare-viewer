#version 450

layout(location = 0) in vec3 position;

layout(location = 0) out vec2 vary_fragcoord;

void main()
{
    vec4 pos = vec4(position, 1.0);
    gl_Position = pos;
    vary_fragcoord = pos.xy * 0.5 + 0.5;
}
