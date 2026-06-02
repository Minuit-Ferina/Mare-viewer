#version 450

layout(set = 0, binding = 0) uniform sampler2D rainbow_map;
layout(set = 0, binding = 1) uniform sampler2D halo_map;

#ifdef HAS_HDRI
layout(set = 0, binding = 2) uniform sampler2D environmentMap;
#endif

layout(set = 1, binding = 1) uniform MareSkyFragmentUniforms
{
    vec4 sky_params0;
    vec4 sky_params1;
#ifdef HAS_HDRI
    mat3 env_mat;
#endif
} u;

layout(location = 0) in vec3 vary_HazeColor;
layout(location = 1) in float vary_LightNormPosDot;

#ifdef HAS_HDRI
layout(location = 2) in vec4 vary_position;
layout(location = 3) in vec3 vary_rel_pos;
#endif

layout(location = 0) out vec4 frag_data[4];

const float PI = 3.14159265;
const float GBUFFER_FLAG_SKIP_ATMOS = 0.0;
const float GBUFFER_FLAG_HAS_HDRI = 1.0;

float moisture_level()
{
    return u.sky_params0.x;
}

float droplet_radius()
{
    return u.sky_params0.y;
}

float ice_level()
{
    return u.sky_params0.z;
}

float sky_hdr_scale()
{
    return u.sky_params0.w;
}

float hdri_split_screen()
{
    return u.sky_params1.x;
}

vec3 rainbow(float d)
{
    d = clamp(-0.575 - d, 0.0, 1.0);

    float interior_coord = max(0.0, d - 0.25) * 4.2857;
    d = clamp(d, 0.0, 0.25) + interior_coord;

    float rad = (droplet_radius() - 5.0) / 1024.0;
    return pow(texture(rainbow_map, vec2(rad + 0.5, d)).rgb, vec3(1.8)) *
        moisture_level();
}

vec3 halo22(float d)
{
    d = clamp(d, 0.1, 1.0);
    float v = sqrt(clamp(1.0 - (d * d), 0.0, 1.0));
    return texture(halo_map, vec2(0.0, v)).rgb * ice_level();
}

void main()
{
    vec3 color;

#ifdef HAS_HDRI
    vec3 frag_coord = vary_position.xyz / vary_position.w;
    if (-frag_coord.x > ((1.0 - hdri_split_screen()) * 2.0 - 1.0))
    {
        vec3 pos = normalize(vary_rel_pos);
        pos = u.env_mat * pos;
        vec2 tex_coord =
            vec2(atan(pos.z, pos.x) + PI, acos(pos.y)) /
            vec2(2.0 * PI, PI);
        color = textureLod(environmentMap, tex_coord.xy, 0.0).rgb *
            sky_hdr_scale();
        color = min(color, vec3(8192.0 * 8192.0 * 16.0));

        frag_data[2] = vec4(0.0, 0.0, 0.0, GBUFFER_FLAG_HAS_HDRI);
    }
    else
#endif
    {
        color = vary_HazeColor;

        float optic_d = vary_LightNormPosDot;
        color.rgb += rainbow(optic_d);
        color.rgb += halo22(optic_d);
        color.rgb *= 2.0;
        color.rgb = clamp(color.rgb, vec3(0.0), vec3(5.0));

        frag_data[2] = vec4(0.0, 0.0, 0.0, GBUFFER_FLAG_SKIP_ATMOS);
    }

    frag_data[1] = vec4(0.0);

#if defined(HAS_EMISSIVE)
    frag_data[0] = vec4(0.0);
    frag_data[3] = vec4(color.rgb, 1.0);
#else
    frag_data[0] = vec4(color.rgb, 1.0);
    frag_data[3] = vec4(0.0);
#endif
}
