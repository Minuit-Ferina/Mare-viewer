#version 450

layout(set = 0, binding = 0) uniform sampler2D lightMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 4) uniform sampler2D depthMap;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

layout(std140, set = 1, binding = 0) uniform MareDeferredBlurLightUniforms
{
    mat4 inverse_projection;
    mat4 shadow_matrix[6];
    vec4 shadow_clip;
    vec4 shadow_settings;
    vec4 shadow_resolution;
    vec4 shadow_runtime;
    vec4 blur_settings;
    vec4 blur_screen;
    vec4 blur_kernel[4];
} u;

vec3 decode_gbuffer_normal(vec4 encoded)
{
    vec3 normal = normalize(encoded.xyz * 2.0 - 1.0);
    if (dot(normal, normal) <= 0.0001)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    return normal;
}

vec4 reconstruct_view_position(vec2 texcoord)
{
    float depth = texture(depthMap, clamp(texcoord, vec2(0.0), vec2(1.0))).r;
    vec2 ndc_xy = texcoord * 2.0 - 1.0;
    vec4 ndc = vec4(ndc_xy, depth * 2.0 - 1.0, 1.0);
    vec4 position = u.inverse_projection * ndc;
    position.xyz /= max(abs(position.w), 0.000001);
    return vec4(position.xyz, 1.0);
}

void main()
{
    vec2 screen_res = max(u.blur_screen.xy, vec2(1.0));
    vec2 tc = clamp(gl_FragCoord.xy / screen_res, vec2(0.0), vec2(1.0));
    vec4 norm = vec4(decode_gbuffer_normal(texture(normalMap, tc)), 0.0);
    vec3 pos = reconstruct_view_position(tc).xyz;
    vec4 ccol = texture(lightMap, tc).rgba;

    vec2 delta = u.blur_settings.xy;
    float dist_factor = u.blur_settings.z;
    float kern_scale = u.blur_settings.w;

    vec2 dlt = kern_scale * delta / (1.0 + norm.xy * norm.xy);
    dlt /= max(-pos.z * dist_factor, 1.0);

    vec2 defined_weight = u.blur_kernel[0].xy;
    vec4 col = defined_weight.xyxx * ccol;

    float pointplanedist_tolerance_pow2 = pos.z * pos.z * 0.00005;

    tc *= screen_res;
    float tc_mod = 0.5 * (tc.x + tc.y);
    tc_mod -= floor(tc_mod);
    tc_mod *= 2.0;
    tc += ((tc_mod - 0.5) * u.blur_kernel[1].z * dlt * 0.5);

    vec3 k[7];
    k[0] = u.blur_kernel[0].xyz;
    k[2] = u.blur_kernel[1].xyz;
    k[4] = u.blur_kernel[2].xyz;
    k[6] = u.blur_kernel[3].xyz;

    k[1] = (k[0] + k[2]) * 0.5;
    k[3] = (k[2] + k[4]) * 0.5;
    k[5] = (k[4] + k[6]) * 0.5;

    for (int i = 1; i < 7; ++i)
    {
        vec2 samptc = tc + k[i].z * dlt * 2.0;
        samptc /= screen_res;
        vec3 samppos = reconstruct_view_position(samptc).xyz;

        float d = dot(norm.xyz, samppos.xyz - pos.xyz);
        if (d * d <= pointplanedist_tolerance_pow2)
        {
            col += texture(lightMap, samptc) * k[i].xyxx;
            defined_weight += k[i].xy;
        }
    }

    for (int i = 1; i < 7; ++i)
    {
        vec2 samptc = tc - k[i].z * dlt * 2.0;
        samptc /= screen_res;
        vec3 samppos = reconstruct_view_position(samptc).xyz;

        float d = dot(norm.xyz, samppos.xyz - pos.xyz);
        if (d * d <= pointplanedist_tolerance_pow2)
        {
            col += texture(lightMap, samptc) * k[i].xyxx;
            defined_weight += k[i].xy;
        }
    }

    col /= max(defined_weight.xyxx, vec4(0.000001));
    frag_color = max(col, vec4(0.0));
}
