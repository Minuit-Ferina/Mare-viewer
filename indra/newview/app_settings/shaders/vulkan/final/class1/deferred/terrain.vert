#version 450

layout(set = 1, binding = 0) uniform MareTerrainUniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 texture_matrix0;
    mat4 normal_matrix;
    vec4 object_plane_s;
    vec4 object_plane_t;
} u;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 3) in vec2 texcoord1;

layout(location = 0) out vec3 vary_position;
layout(location = 1) out vec3 vary_normal;
layout(location = 2) out vec4 vary_texcoord0;
layout(location = 3) out vec4 vary_texcoord1;

vec2 texgen_object(vec4 vpos, mat4 mat, vec4 tp0, vec4 tp1)
{
    vec4 tcoord;
    tcoord.x = dot(vpos, tp0);
    tcoord.y = dot(vpos, tp1);
    tcoord.z = 0.0;
    tcoord.w = 1.0;

    tcoord = mat * tcoord;

    return tcoord.xy;
}

void main()
{
    vec4 pre_pos = vec4(position.xyz, 1.0);
    gl_Position = u.modelview_projection_matrix * pre_pos;
    gl_Position.y = -gl_Position.y;

    vary_position = (u.modelview_matrix * pre_pos).xyz;
    vary_normal = normalize((u.normal_matrix * vec4(normal, 0.0)).xyz);

    vary_texcoord0.xy = texgen_object(
        pre_pos,
        u.texture_matrix0,
        u.object_plane_s,
        u.object_plane_t);

    vec4 t = vec4(texcoord1, 0.0, 1.0);
    vary_texcoord0.zw = t.xy;
    vary_texcoord1.xy = t.xy - vec2(2.0, 0.0);
    vary_texcoord1.zw = t.xy - vec2(1.0, 0.0);
}
