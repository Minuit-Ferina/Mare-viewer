#version 450

layout(set = 1, binding = 0) uniform MareSkyVertexUniforms
{
    mat4 modelview_projection_matrix;
    vec4 cam_pos_local;
    vec4 lightnorm;
    vec4 sunlight_color;
    vec4 moonlight_color;
    vec4 ambient_color;
    vec4 blue_horizon;
    vec4 blue_density;
    vec4 glow;
    vec4 sky_params0;
    vec4 sky_params1;
    vec4 sky_params2;
} u;

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 vary_HazeColor;
layout(location = 1) out float vary_LightNormPosDot;

#ifdef HAS_HDRI
layout(location = 2) out vec4 vary_position;
layout(location = 3) out vec3 vary_rel_pos;
#endif

float haze_horizon()
{
    return u.sky_params0.x;
}

float haze_density()
{
    return u.sky_params0.y;
}

float cloud_shadow()
{
    return u.sky_params0.z;
}

float density_multiplier()
{
    return u.sky_params0.w;
}

float distance_multiplier()
{
    return u.sky_params1.x;
}

float max_y()
{
    return u.sky_params1.y;
}

float sun_moon_glow_factor()
{
    return u.sky_params1.z;
}

int sun_up_factor()
{
    return int(u.sky_params1.w + 0.5);
}

int cube_snapshot()
{
    return int(u.sky_params2.x + 0.5);
}

void main()
{
    vec4 pos = u.modelview_projection_matrix * vec4(position.xyz, 1.0);
    gl_Position = pos;
    gl_Position.y = -gl_Position.y;

    vec3 rel_pos = position.xyz - u.cam_pos_local.xyz + vec3(0.0, 50.0, 0.0);

#ifdef HAS_HDRI
    vary_rel_pos = rel_pos;
    vary_position = pos;
#endif

    if (rel_pos.y > 0.0)
    {
        rel_pos *= max_y() / rel_pos.y;
    }
    if (rel_pos.y < 0.0)
    {
        rel_pos *= -32000.0 / rel_pos.y;
    }

    vec3 rel_pos_norm = normalize(rel_pos);
    float rel_pos_len = length(rel_pos);

    float rel_pos_lightnorm_dot = dot(rel_pos_norm, u.lightnorm.xyz);
    vary_LightNormPosDot = rel_pos_lightnorm_dot;

    vec3 sunlight = sun_up_factor() == 1 ?
        u.sunlight_color.rgb :
        u.moonlight_color.rgb * 0.7;

    vec3 light_atten =
        (u.blue_density.rgb + vec3(haze_density() * 0.25)) *
        (density_multiplier() * max_y());

    vec3 combined_haze =
        max(abs(u.blue_density.rgb) + vec3(abs(haze_density())), vec3(1e-6));
    vec3 blue_weight = u.blue_density.rgb / combined_haze;
    vec3 haze_weight = vec3(haze_density()) / combined_haze;

    float off_axis =
        1.0 / max(1e-6, max(0.0, rel_pos_norm.y) + u.lightnorm.y);
    sunlight *= exp(-light_atten * off_axis);

    float density_dist = rel_pos_len * density_multiplier();
    combined_haze = exp(-combined_haze * density_dist);

    float haze_glow = 1.0 - rel_pos_lightnorm_dot;
    haze_glow = max(haze_glow, 0.001);
    haze_glow *= u.glow.x;
    haze_glow = pow(haze_glow, u.glow.z);
    haze_glow = sun_moon_glow_factor() < 1.0 ?
        0.0 :
        sun_moon_glow_factor() * (haze_glow + 0.25);

    vec3 color =
        u.blue_horizon.rgb * blue_weight * (sunlight + u.ambient_color.rgb) +
        (haze_horizon() * haze_weight) *
            (sunlight * haze_glow + u.ambient_color.rgb);

    color *= 1.0 - combined_haze;

    vec3 ambient =
        u.ambient_color.rgb +
        max(vec3(0.0), 1.0 - u.ambient_color.rgb) * cloud_shadow() * 0.5;

    sunlight *= max(0.0, 1.0 - cloud_shadow());

    vec3 add_below_cloud =
        u.blue_horizon.rgb * blue_weight * (sunlight + ambient) +
        (haze_horizon() * haze_weight) * (sunlight * haze_glow + ambient);

    combined_haze = sqrt(combined_haze);
    color += (add_below_cloud - color) * (1.0 - sqrt(combined_haze));

    vary_HazeColor = color;
}
