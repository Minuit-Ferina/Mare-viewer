#version 450

layout(location = 0) in vec3 position;

layout(location = 0) out vec2 vary_texcoord0;

void main()
{
    gl_Position = vec4(position, 1.0);
    vary_texcoord0 = position.xy * 0.5 + 0.5;
}
