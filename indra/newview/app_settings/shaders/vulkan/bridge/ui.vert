#version 450

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texcoord0;
layout(location = 6) in vec4 diffuse_color;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;

void main()
{
    gl_Position = vec4(position.x, -position.y, position.z, 1.0);
    vary_texcoord0 = texcoord0;
    vertex_color = diffuse_color;
}
