#version 450

layout(set = 0, binding = 0) uniform sampler2D emissiveRect;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
    vec4 params1;
} pc;

layout(location = 0) out vec4 frag_color;

void main()
{
    float exposure_luminance = textureLod(emissiveRect, vec2(0.5), 8.0).r;
    float max_luminance = pc.params0.x;
    exposure_luminance = clamp(exposure_luminance, 0.0, max_luminance);
    exposure_luminance /= max_luminance;
    exposure_luminance = pow(exposure_luminance, 2.0);

    float exposure = mix(pc.params0.z, pc.params0.y, exposure_luminance);
    frag_color = max(vec4(exposure, exposure, exposure, pc.params1.x), vec4(0.0));
}
