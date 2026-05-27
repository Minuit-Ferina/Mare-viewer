#version 450

layout(push_constant) uniform MareWorldPushConstants
{
    mat4 modelview_projection_matrix;
    vec4 params;
    vec4 terrain_params;
} pc;

layout(location = 0) in vec3 position;
layout(location = 3) in vec2 texcoord1;

layout(location = 0) out vec2 vary_detail_texcoord;
layout(location = 1) out vec2 vary_alpha_01;
layout(location = 2) out vec2 vary_alpha_23;
layout(location = 3) out vec2 vary_alpha_final;

void main()
{
    gl_Position = pc.modelview_projection_matrix * vec4(position, 1.0);
    gl_Position.y = -gl_Position.y;

    vary_detail_texcoord =
        position.xy * pc.terrain_params.x +
        pc.terrain_params.yz;
    vary_alpha_01 = texcoord1;
    vary_alpha_23 = texcoord1 - vec2(2.0, 0.0);
    vary_alpha_final = texcoord1 - vec2(1.0, 0.0);
}
