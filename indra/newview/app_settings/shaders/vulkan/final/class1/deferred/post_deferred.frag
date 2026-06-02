#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;

layout(push_constant) uniform MarePostProcessPushConstants
{
    vec4 params0;
    vec4 params1;
} pc;

layout(location = 0) in vec2 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

float max_cof()
{
    return pc.params0.x;
}

vec2 screen_res()
{
    return pc.params1.xy;
}

vec3 clampHDRRange(vec3 color)
{
    color = mix(color, vec3(1.0), isinf(color));
    color = mix(color, vec3(0.0), isnan(color));
    return clamp(color, vec3(0.0), vec3(11.2));
}

void dofSample(inout vec4 diff, inout float w, float min_sc, vec2 tc)
{
    vec4 s = texture(diffuseRect, tc);

    float sc = abs(s.a * 2.0 - 1.0) * max_cof();

    if (sc > min_sc)
    {
        float wg = 0.25;
        wg += s.r + s.g + s.b;

        diff += wg * s;
        w += wg;
    }
}

void dofSampleNear(inout vec4 diff, inout float w, float min_sc, vec2 tc)
{
    vec4 s = texture(diffuseRect, tc);

    float wg = 0.25;
    wg += s.r + s.g + s.b;

    diff += wg * s;
    w += wg;
}

void main()
{
    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);

    float w = 1.0;
    float sc = (diff.a * 2.0 - 1.0) * max_cof();
    float PI = 3.14159265358979323846264;

    if (sc > 0.5)
    {
        while (sc > 0.5)
        {
            int its = int(max(1.0, sc * 3.7));
            for (int i = 0; i < its; ++i)
            {
                float ang = sc + float(i) * 2.0 * PI / float(its);
                float samp_x = sc * sin(ang);
                float samp_y = sc * cos(ang);
                dofSampleNear(
                    diff,
                    w,
                    sc,
                    vary_fragcoord.xy + vec2(samp_x, samp_y) / screen_res());
            }
            sc -= 1.0;
        }
    }
    else if (sc < -0.5)
    {
        sc = abs(sc);
        while (sc > 0.5)
        {
            int its = int(max(1.0, sc * 3.7));
            for (int i = 0; i < its; ++i)
            {
                float ang = sc + float(i) * 2.0 * PI / float(its);
                float samp_x = sc * sin(ang);
                float samp_y = sc * cos(ang);
                dofSample(
                    diff,
                    w,
                    sc,
                    vary_fragcoord.xy + vec2(samp_x, samp_y) / screen_res());
            }
            sc -= 1.0;
        }
    }

    diff /= w;
    diff.rgb = clampHDRRange(diff.rgb);
    frag_color = diff;
}
