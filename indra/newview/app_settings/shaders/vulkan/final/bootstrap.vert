#version 450

vec2 fallback_positions[3] = vec2[]
(
    vec2(-0.6, -0.6),
    vec2(0.6, -0.6),
    vec2(0.0, 0.6)
);

void main()
{
    gl_Position = vec4(fallback_positions[gl_VertexIndex], 0.0, 1.0);
}
