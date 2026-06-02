#version 450

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 0) mat4 modelview_projection_matrix;
    layout(offset = 96) vec4 texture_transform_s;
    layout(offset = 112) vec4 texture_transform_t;
    layout(offset = 192) vec4 water_wave_dirs;
    layout(offset = 208) vec4 water_time_height_fog;
    layout(offset = 224) vec4 water_eye_vec;
    layout(offset = 352) mat4 modelview_matrix;
} pc;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;
layout(location = 3) in vec2 texcoord1;
layout(location = 4) in vec2 texcoord2;
layout(location = 6) in vec4 diffuse_color;
layout(location = 8) in vec4 tangent;
layout(location = 9) in float weight;
layout(location = 10) in vec4 weight4;
layout(location = 13) in uint texture_index;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;
layout(location = 3) out vec3 vary_normal;
layout(location = 4) out vec4 vary_tangent;
layout(location = 5) out vec3 vary_position;
layout(location = 6) out vec4 refCoord;
layout(location = 7) out vec4 littleWave;
layout(location = 8) out vec4 view;

vec2 texture_matrix_transform(vec2 texcoord)
{
    vec4 tc = vec4(texcoord, 0.0, 1.0);
    return vec2(
        dot(tc, pc.texture_transform_s),
        dot(tc, pc.texture_transform_t));
}

vec3 safe_normalize(vec3 value, vec3 fallback)
{
    if (dot(value, value) <= 0.0001)
    {
        return fallback;
    }
    return normalize(value);
}

void main()
{
    vec4 object_position = vec4(position.xyz, 1.0);
    vec4 eye_position = pc.modelview_matrix * object_position;
    vec4 clip_position = pc.modelview_projection_matrix * object_position;

    vec3 source_eye_vec = pc.water_eye_vec.xyz;
    vec3 object_to_eye = position.xyz - source_eye_vec;
    float distance_xy = length(object_to_eye.xy);
    float limited_distance = min(distance_xy, 2560.0);

    vec3 wave_position = position.xyz;
    if (distance_xy > 0.0001)
    {
        wave_position.xy =
            source_eye_vec.xy +
            object_to_eye.xy / distance_xy * limited_distance;
    }
    wave_position.x +=
        (cos(wave_position.x * 0.08) + sin(wave_position.y * 0.02)) * 6.0;

    vec2 wave_dir1 = pc.water_wave_dirs.xy;
    vec2 wave_dir2 = pc.water_wave_dirs.zw;
    float time = pc.water_time_height_fog.x;
    vec2 big_wave = wave_position.xy * vec2(0.04, 0.04) + wave_dir1 * time * 0.055;

    gl_Position = clip_position;
    gl_Position.y = -gl_Position.y;

    vertex_color = diffuse_color;
    vary_texcoord0 = texture_matrix_transform(texcoord0);
    vary_position = eye_position.xyz;
    vary_normal =
        safe_normalize(mat3(pc.modelview_matrix) * vec3(0.0, 0.0, 1.0),
            vec3(0.0, 0.0, 1.0));
    vary_tangent = vec4(
        safe_normalize(mat3(pc.modelview_matrix) * vec3(1.0, 0.0, 0.0),
            vec3(1.0, 0.0, 0.0)),
        1.0);

    refCoord.xyz = clip_position.xyz + vec3(0.0, 0.0, 0.2);
    refCoord.w = big_wave.x;
    littleWave.xy =
        wave_position.xy * vec2(0.45, 0.9) + wave_dir2 * time * 0.13;
    littleWave.zw =
        wave_position.xy * vec2(0.1, 0.2) + wave_dir1 * time * 0.1;
    view.xyz = object_to_eye;
    view.w = big_wave.y;
}
