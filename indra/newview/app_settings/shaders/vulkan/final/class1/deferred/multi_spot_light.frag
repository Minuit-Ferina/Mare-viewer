#version 450

// Faithful source-level Vulkan port of class1/deferred/multiSpotLightF.glsl.
// Class 1 multi-spotlights preserve the no-shadow legacy projected-light path.

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D specularRect;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D depthMap;
layout(set = 0, binding = 4) uniform samplerCube environmentMap;
layout(set = 0, binding = 5) uniform sampler2D noiseMap;
layout(set = 0, binding = 6) uniform sampler2D projectionMap;
layout(set = 0, binding = 7) uniform sampler2D lightFunc;

layout(set = 1, binding = 0) uniform MareMultiSpotLightClass1Uniforms
{
    mat4 inv_proj;
    mat4 proj_mat;
    vec4 proj_p_near;              // xyz = proj_p, w = proj_near
    vec4 proj_n_focus;             // xyz = proj_n, w = proj_focus
    vec4 proj_lod_range_ambiance;  // x = proj_lod, y = proj_range, z = proj_ambient_lod, w = proj_ambiance
    vec4 near_far_sun_wash_pad;    // x = near_clip, y = far_clip, z = sun_wash
    vec4 proj_origin_size;         // xyz = proj_origin, w = size
    vec4 center_falloff;           // xyz = center, w = falloff
    vec4 color_screen_x;           // rgb = color, w = screen_res.x
    vec4 screen_y_pad;             // x = screen_res.y
} u;

layout(location = 0) in vec4 vary_fragcoord;

layout(location = 0) out vec4 frag_color;

vec3 srgb_to_linear(vec3 c)
{
    bvec3 cutoff = lessThanEqual(c, vec3(0.04045));
    vec3 low = c / 12.92;
    vec3 high = pow((c + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
}

vec4 decodeNormal(vec4 norm)
{
    vec2 fenc = norm.xy * 4.0 - 2.0;
    float f = dot(fenc, fenc);
    float g = sqrt(max(1.0 - f / 4.0, 0.0));
    vec4 n;
    n.xy = fenc * g;
    n.z = 1.0 - f / 2.0;
    n.w = norm.w;
    return n;
}

vec4 texture2DLodSpecular(vec2 tc, float lod)
{
    vec4 ret = textureLod(projectionMap, tc, lod);
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = vec2(0.5) - abs(tc - vec2(0.5));
    float det = min(lod / (u.proj_lod_range_ambiance.x * 0.5), 1.0);
    float d = min(dist.x, dist.y);
    d *= min(1.0, d * (u.proj_lod_range_ambiance.x - lod));
    float edge = 0.25 * det;
    ret *= clamp(d / edge, 0.0, 1.0);
    return ret;
}

vec4 texture2DLodDiffuse(vec2 tc, float lod)
{
    vec4 ret = textureLod(projectionMap, tc, lod);
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = vec2(0.5) - abs(tc - vec2(0.5));
    float det = min(lod / (u.proj_lod_range_ambiance.x * 0.5), 1.0);
    float d = min(dist.x, dist.y);
    float edge = 0.25 * det;
    ret *= clamp(d / edge, 0.0, 1.0);
    return ret;
}

vec4 texture2DLodAmbient(vec2 tc, float lod)
{
    vec4 ret = textureLod(projectionMap, tc, lod);
    ret.rgb = srgb_to_linear(ret.rgb);

    vec2 dist = tc - vec2(0.5);
    float d = dot(dist, dist);
    ret *= min(clamp((0.25 - d) / 0.25, 0.0, 1.0), 1.0);
    return ret;
}

vec2 getScreenCoordinate(vec2 screenpos)
{
    return screenpos.xy * 2.0 - vec2(1.0);
}

float getDepth(vec2 tc)
{
    return texture(depthMap, tc).r;
}

vec4 getPosition(vec2 pos_screen)
{
    float depth = getDepth(pos_screen);
    vec2 sc = getScreenCoordinate(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0 * depth - 1.0, 1.0);
    vec4 pos = u.inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}

vec3 getNorm(vec2 pos_screen)
{
    return normalize(decodeNormal(texture(normalMap, pos_screen)).xyz);
}

vec2 getScreenCoord(vec4 clip)
{
    vec4 ndc = clip;
    ndc.xyz /= clip.w;
    return ndc.xy * 0.5 + vec2(0.5);
}

void main()
{
    vec3 col = vec3(0.0);

#if defined(LOCAL_LIGHT_KILL)
    discard;
#else
    vec2 tc = getScreenCoord(vary_fragcoord);
    vec2 screen_res = vec2(u.color_screen_x.w, u.screen_y_pad.x);
    vec3 pos = getPosition(tc).xyz;
    vec3 lv = u.center_falloff.xyz - pos.xyz;
    float dist = length(lv);
    dist /= u.proj_origin_size.w;
    if (dist > 1.0)
    {
        discard;
    }

    float envIntensity = texture(normalMap, tc).z;
    vec3 norm = getNorm(tc);
    float l_dist = -dot(lv, u.proj_n_focus.xyz);

    vec4 proj_tc = u.proj_mat * vec4(pos.xyz, 1.0);
    if (proj_tc.z < 0.0)
    {
        discard;
    }
    proj_tc.xyz /= proj_tc.w;

    float fa = u.center_falloff.w + 1.0;
    float dist_atten = clamp(1.0 - (dist - 1.0 * (1.0 - fa)) / fa, 0.0, 1.0);
    dist_atten *= dist_atten;
    dist_atten *= 2.0;

    if (dist_atten <= 0.0)
    {
        discard;
    }

    float noise = texture(noiseMap, tc * screen_res / 128.0).b;
    dist_atten *= noise;

    lv = u.proj_origin_size.xyz - pos.xyz;
    lv = normalize(lv);
    float da = dot(norm, lv);

    vec3 diff_tex = texture(diffuseRect, tc).rgb;
    vec3 dlit = vec3(0.0);

    if (proj_tc.z > 0.0 &&
        proj_tc.x < 1.0 &&
        proj_tc.y < 1.0 &&
        proj_tc.x > 0.0 &&
        proj_tc.y > 0.0)
    {
        float lit = 0.0;
        float amb_da = u.proj_lod_range_ambiance.w;

        if (da > 0.0)
        {
            float diff = clamp((l_dist - u.proj_n_focus.w) / u.proj_lod_range_ambiance.y, 0.0, 1.0);
            float lod = diff * u.proj_lod_range_ambiance.x;
            vec4 plcol = texture2DLodDiffuse(proj_tc.xy, lod);

            dlit = u.color_screen_x.rgb * plcol.rgb * plcol.a;
            lit = da * dist_atten;
            col = dlit * lit * diff_tex;
            amb_da += (da * 0.5) * u.proj_lod_range_ambiance.w;
        }

        vec4 amb_plcol = texture2DLodAmbient(proj_tc.xy, u.proj_lod_range_ambiance.x);
        amb_da += (da * da * 0.5 + 0.5) * u.proj_lod_range_ambiance.w;
        amb_da *= dist_atten * noise;
        amb_da = min(amb_da, 1.0 - lit);
        col += amb_da * u.color_screen_x.rgb * diff_tex * amb_plcol.rgb * amb_plcol.a;
    }

    vec4 spec = texture(specularRect, tc);
    if (spec.a > 0.0)
    {
        dlit *= min(da * 6.0, 1.0) * dist_atten;

        vec3 npos = -normalize(pos);
        vec3 h = normalize(lv + npos);
        float nh = dot(norm, h);
        float nv = dot(norm, npos);
        float vh = dot(npos, h);
        float fres = pow(1.0 - dot(h, npos), 5.0) * 0.4 + 0.5;
        float gtdenom = 2.0 * nh;
        float gt = max(0.0, min(gtdenom * nv / vh, gtdenom * da / vh));

        if (nh > 0.0)
        {
            float scol = fres * texture(lightFunc, vec2(nh, spec.a)).r * gt / (nh * da);
            col += dlit * scol * spec.rgb;
        }
    }

    if (envIntensity > 0.0)
    {
        vec3 ref = reflect(normalize(pos), norm);
        vec3 pdelta = u.proj_p_near.xyz - pos;
        float ds = dot(ref, u.proj_n_focus.xyz);

        if (ds < 0.0)
        {
            vec3 pfinal = pos + ref * dot(pdelta, u.proj_n_focus.xyz) / ds;
            vec4 stc = u.proj_mat * vec4(pfinal.xyz, 1.0);

            if (stc.z > 0.0)
            {
                stc /= stc.w;

                if (stc.x < 1.0 &&
                    stc.y < 1.0 &&
                    stc.x > 0.0 &&
                    stc.y > 0.0)
                {
                    col += u.color_screen_x.rgb *
                        texture2DLodSpecular(stc.xy, (1.0 - spec.a) * (u.proj_lod_range_ambiance.x * 0.6)).rgb *
                        envIntensity;
                }
            }
        }
    }
#endif

    frag_color.rgb = col;
    frag_color.a = 0.0;
}
