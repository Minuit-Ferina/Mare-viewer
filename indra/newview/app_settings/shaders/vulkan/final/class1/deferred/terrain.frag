#version 450

layout(set = 0, binding = 0) uniform sampler2D detail_0;
layout(set = 0, binding = 1) uniform sampler2D detail_1;
layout(set = 0, binding = 2) uniform sampler2D detail_2;
layout(set = 0, binding = 3) uniform sampler2D detail_3;
layout(set = 0, binding = 4) uniform sampler2D alpha_ramp;

layout(location = 0) in vec3 vary_position;
layout(location = 1) in vec3 vary_normal;
layout(location = 2) in vec4 vary_texcoord0;
layout(location = 3) in vec4 vary_texcoord1;

layout(location = 0) out vec4 frag_data[4];

vec4 encode_normal(vec3 n, float env, float gbuffer_flag)
{
    vec3 encoded = normalize(n) * 0.5 + 0.5;
    return vec4(encoded.xy, env, gbuffer_flag);
}

void main()
{
    vec4 color0 = texture(detail_0, vary_texcoord0.xy);
    vec4 color1 = texture(detail_1, vary_texcoord0.xy);
    vec4 color2 = texture(detail_2, vary_texcoord0.xy);
    vec4 color3 = texture(detail_3, vary_texcoord0.xy);

    float alpha1 = texture(alpha_ramp, vary_texcoord0.zw).a;
    float alpha2 = texture(alpha_ramp, vary_texcoord1.xy).a;
    float alpha_final = texture(alpha_ramp, vary_texcoord1.zw).a;
    vec4 out_color = mix(
        mix(color3, color2, alpha2),
        mix(color1, color0, alpha1),
        alpha_final);

    out_color.a = 0.0;

    frag_data[0] = max(out_color, vec4(0.0));
    frag_data[1] = vec4(0.0, 0.0, 0.0, -1.0);
    frag_data[2] = encode_normal(vary_normal, 0.0, 1.0);
    frag_data[3] = vec4(0.0);
}
