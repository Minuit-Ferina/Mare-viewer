// Vulkan final shader source port.
// Source OpenGL shader: class1/deferred/impostorF.glsl
// This file preserves the source shader's role while the final Vulkan
// renderer pipeline contracts are completed.

#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 2) uniform sampler2D specularMap;
layout(set = 0, binding = 3) uniform sampler2D tex3;
layout(set = 0, binding = 4) uniform sampler2D tex4;
layout(set = 0, binding = 5) uniform sampler2D tex5;
layout(set = 0, binding = 6) uniform sampler2D tex6;
layout(set = 0, binding = 7) uniform sampler2D tex7;

layout(push_constant) uniform MareGeneratedFragmentConstants
{
    float minimum_alpha;
    vec3 _pad0;
    vec4 color;
    vec4 params;
} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 2) in vec3 vary_normal;
layout(location = 3) in vec3 vary_position;
layout(location = 4) flat in uint vary_texture_index;
layout(location = 5) in vec4 vary_tangent;
layout(location = 6) in vec2 vary_texcoord1;
layout(location = 7) in vec2 vary_texcoord2;
layout(location = 8) in vec4 vertex_position;

layout(location = 0) out vec4 frag_data[4];

void main()
{
    vec4 color = texture(diffuseMap, vary_texcoord0.xy) * vertex_color * max(pc.color, vec4(1.0));

    if (color.a < pc.minimum_alpha)
    {
        discard;
    }

    vec4 normal = texture(normalMap, vary_texcoord0.xy);
    vec4 specular = texture(specularMap, vary_texcoord0.xy);

    frag_data[0] = vec4(color.rgb, 0.0);
    frag_data[1] = specular;
    frag_data[2] = vec4(normal.xyz, 1.0);
    frag_data[3] = vec4(0.0);
}
