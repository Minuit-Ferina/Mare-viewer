#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;

layout(location = 0) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

void main()
{
    vec3 color = texture(diffuseRect, vary_texcoord0).rgb;
    frag_color = vec4(color, dot(color, vec3(0.299, 0.587, 0.144)));
}
