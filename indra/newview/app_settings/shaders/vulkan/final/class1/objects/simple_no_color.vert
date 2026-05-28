#version 450

layout(set = 1, binding = 0) uniform MareSimpleUniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 texture_matrix0;
    mat4 normal_matrix;
    vec4 color;
} u;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;

vec4 calc_lighting(vec3 pos, vec3 norm, vec4 color)
{
    vec3 light_dir = normalize(vec3(0.35, 0.45, 0.82));
    float diffuse = max(dot(norm, light_dir), 0.0);
    return vec4(color.rgb * (0.35 + diffuse * 0.65), color.a);
}

void main()
{
    vec4 pos = u.modelview_matrix * vec4(position.xyz, 1.0);
    gl_Position = u.modelview_projection_matrix * vec4(position.xyz, 1.0);
    gl_Position.y = -gl_Position.y;
    vary_texcoord0 = (u.texture_matrix0 * vec4(texcoord0, 0.0, 1.0)).xy;

    vec3 norm = normalize((u.normal_matrix * vec4(normal, 0.0)).xyz);
    vertex_color = calc_lighting(pos.xyz, norm, u.color);
}
