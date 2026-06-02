#version 450

layout(location = 0) in vec3 position;

layout(location = 0) out vec4 vary_fragcoord;

void main()
{
    vec4 pos = vec4(position.xyz, 1.0);
    vary_fragcoord = pos;

    gl_Position = pos;
    gl_Position.y = -gl_Position.y;
}
