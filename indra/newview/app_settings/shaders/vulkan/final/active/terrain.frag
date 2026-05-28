#version 450

layout(set = 0, binding = 0) uniform sampler2D detail0;
layout(set = 0, binding = 1) uniform sampler2D detail1;
layout(set = 0, binding = 2) uniform sampler2D detail2;
layout(set = 0, binding = 3) uniform sampler2D detail3;
layout(set = 0, binding = 4) uniform sampler2D alphaRamp;

layout(location = 0) in vec2 vary_detail_texcoord;
layout(location = 1) in vec2 vary_alpha_01;
layout(location = 2) in vec2 vary_alpha_23;
layout(location = 3) in vec2 vary_alpha_final;

layout(location = 0) out vec4 frag_color;

void main()
{
    vec4 color0 = texture(detail0, vary_detail_texcoord);
    vec4 color1 = texture(detail1, vary_detail_texcoord);
    vec4 color2 = texture(detail2, vary_detail_texcoord);
    vec4 color3 = texture(detail3, vary_detail_texcoord);

    float alpha1 = texture(alphaRamp, vary_alpha_01).a;
    float alpha2 = texture(alphaRamp, vary_alpha_23).a;
    float alphaFinal = texture(alphaRamp, vary_alpha_final).a;

    vec4 color =
        mix(
            mix(color3, color2, alpha2),
            mix(color1, color0, alpha1),
            alphaFinal);

    frag_color = vec4(max(color.rgb, vec3(0.0)), 1.0);
}
