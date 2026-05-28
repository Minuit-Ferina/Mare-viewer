#version 450

layout(push_constant) uniform MareInterfacePushConstants
{
    layout(offset = 128) vec4 color;
    vec4 clip_plane;
} pc;

layout(location = 0) in vec3 vary_position;

layout(location = 0) out vec4 frag_color;

void main()
{
    if (dot(vary_position, pc.clip_plane.xyz) + pc.clip_plane.w < 0.0)
    {
        discard;
    }

    frag_color = max(pc.color, vec4(0.0));
}
