#version 450

// Faithful source-level Vulkan port of class3/deferred/spotLightF.glsl.
// Class 3 preserves the PBR and legacy projected-light branches.

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D specularRect;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D depthMap;
layout(set = 0, binding = 4) uniform samplerCube environmentMap;
layout(set = 0, binding = 5) uniform sampler2D lightMap;
layout(set = 0, binding = 6) uniform sampler2D noiseMap;
layout(set = 0, binding = 7) uniform sampler2D projectionMap;
layout(set = 0, binding = 8) uniform sampler2D lightFunc;

layout(set = 1, binding = 0) uniform MareSpotLightClass3Uniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    vec4 center_size;               // xyz = light center, w = light size
    mat4 inv_proj;
    mat4 proj_mat;
    vec4 proj_p_near;              // xyz = proj_p, w = proj_near
    vec4 proj_n_focus;             // xyz = proj_n, w = proj_focus
    vec4 proj_lod_range_ambiance;  // x = proj_lod, y = proj_range, z = proj_ambient_lod, w = proj_ambiance
    vec4 near_far_sun_shadow;      // x = near_clip, y = far_clip, z = sun_wash, w = shadow_fade
    ivec4 shadow_indices;          // x = proj_shadow_idx
    vec4 proj_origin_size;         // xyz = proj_origin, w = size
    vec4 color_falloff;            // rgb = color, a = falloff
    vec4 screen_res_classic;       // xy = screen_res, z = classic_mode
} u;

layout(location = 0) in vec4 vary_fragcoord;
layout(location = 1) in vec3 trans_center;

layout(location = 0) out vec4 frag_color;

const float GBUFFER_FLAG_HAS_PBR = 0.67;
const float M_PI = 3.14159265;

struct GBufferInfo
{
    vec4 albedo;
    vec3 normal;
    vec4 specular;
    float envIntensity;
    float gbufferFlag;
    vec4 emissive;
};

bool get_gbuffer_flag(float data, float flag)
{
    return abs(data - flag) < 0.1;
}

vec3 srgb_to_linear(vec3 c)
{
    bvec3 cutoff = lessThanEqual(c, vec3(0.04045));
    vec3 low = c / 12.92;
    vec3 high = pow((c + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
}

vec4 decodeNormal(vec4 norm)
{
    vec3 n = normalize(norm.xyz * 2.0 - 1.0);
    if (dot(n, n) <= 0.0001)
    {
        n = vec3(0.0, 0.0, 1.0);
    }
    return vec4(n, norm.w);
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

vec4 getTexture2DLodDiffuse(vec2 tc, float lod)
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

vec4 getTexture2DLodAmbient(vec2 tc, float lod)
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

vec2 getScreenCoord(vec4 clip)
{
    vec4 ndc = clip;
    ndc.xyz /= clip.w;
    return ndc.xy * 0.5 + vec2(0.5);
}

vec4 getNormRaw(vec2 screenpos)
{
    return texture(normalMap, screenpos.xy);
}

GBufferInfo getGBuffer(vec2 screenpos)
{
    GBufferInfo ret;
    vec4 normInfo = getNormRaw(screenpos);
    ret.albedo = texture(diffuseRect, screenpos.xy);
    ret.normal = decodeNormal(normInfo).xyz;
    ret.specular = texture(specularRect, screenpos.xy);
    ret.envIntensity = ret.albedo.a;
    ret.gbufferFlag = normInfo.w;
    ret.emissive = vec4(0.0);
    return ret;
}

float calcLegacyDistanceAttenuation(float distance, float falloff)
{
    float dist_atten = 1.0 - clamp((distance + falloff) / (1.0 + falloff), 0.0, 1.0);
    dist_atten *= dist_atten;
    dist_atten *= 2.0;
    return dist_atten;
}

void calcHalfVectors(vec3 lv, vec3 n, vec3 v,
                     out vec3 h, out vec3 l, out float nh, out float nl,
                     out float nv, out float vh, out float lightDist)
{
    l = normalize(lv);
    h = normalize(l + v);

    float eps = 0.000001;
    nh = clamp(dot(n, h), eps, 1.0);
    nl = clamp(dot(n, l), eps, 1.0);
    nv = clamp(dot(n, v), eps, 1.0);
    vh = clamp(dot(v, h), eps, 1.0);
    lightDist = length(lv);
}

bool clipProjectedLightVars(vec3 center, vec3 pos, out float dist, out float l_dist, out vec3 lv, out vec4 proj_tc)
{
    lv = center - pos.xyz;
    dist = length(lv);
    bool clipped = dist >= u.proj_origin_size.w;
    if (!clipped)
    {
        dist /= u.proj_origin_size.w;
        l_dist = -dot(lv, u.proj_n_focus.xyz);
        proj_tc = u.proj_mat * vec4(pos.xyz, 1.0);
        clipped = proj_tc.z < 0.0;
        proj_tc.xyz /= proj_tc.w;
    }
    return clipped;
}

vec3 getProjectedLightAmbiance(float amb_da, float attenuation, float lit, float nl, float noise, vec2 projected_uv)
{
    vec4 amb_plcol = getTexture2DLodAmbient(projected_uv, u.proj_lod_range_ambiance.x);
    vec3 amb_rgb = amb_plcol.rgb * amb_plcol.a;
    amb_da += u.proj_lod_range_ambiance.w;
    amb_da += (nl * nl * 0.5 + 0.5) * u.proj_lod_range_ambiance.w;
    amb_da *= attenuation * noise;
    amb_da = min(amb_da, 1.0 - lit);
    return amb_da * u.color_falloff.rgb * amb_rgb;
}

vec3 getProjectedLightDiffuseColor(float light_distance, vec2 projected_uv)
{
    float diff = clamp((light_distance - u.proj_n_focus.w) / u.proj_lod_range_ambiance.y, 0.0, 1.0);
    float lod = diff * u.proj_lod_range_ambiance.x;
    vec4 plcol = getTexture2DLodDiffuse(projected_uv.xy, lod);
    return u.color_falloff.rgb * plcol.rgb * plcol.a;
}

struct PBRInfo
{
    float NdotL;
    float NdotV;
    float NdotH;
    float LdotH;
    float VdotH;
    float perceptualRoughness;
    float metalness;
    vec3 reflectance0;
    vec3 reflectance90;
    float alphaRoughness;
    vec3 diffuseColor;
    vec3 specularColor;
};

vec3 diffuse_brdf(PBRInfo pbrInputs)
{
    return pbrInputs.diffuseColor / M_PI;
}

vec3 specularReflection(PBRInfo pbrInputs)
{
    return pbrInputs.reflectance0 +
        (pbrInputs.reflectance90 - pbrInputs.reflectance0) *
        pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

float geometricOcclusion(PBRInfo pbrInputs)
{
    float NdotL = pbrInputs.NdotL;
    float NdotV = pbrInputs.NdotV;
    float r = pbrInputs.alphaRoughness;
    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

float microfacetDistribution(PBRInfo pbrInputs)
{
    float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
    float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
    return roughnessSq / (M_PI * f * f);
}

void pbrPunctual(vec3 diffuseColor, vec3 specularColor,
                 float perceptualRoughness,
                 float metallic,
                 vec3 n,
                 vec3 v,
                 vec3 l,
                 out float nl,
                 out vec3 diff,
                 out vec3 spec)
{
    perceptualRoughness = max(perceptualRoughness, 8.0 / 255.0);

    float alphaRoughness = perceptualRoughness * perceptualRoughness;
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 h = normalize(l + v);

    float NdotL = clamp(dot(n, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    PBRInfo pbrInputs = PBRInfo(
        NdotL,
        NdotV,
        NdotH,
        LdotH,
        VdotH,
        perceptualRoughness,
        metallic,
        specularColor.rgb,
        vec3(1.0) * reflectance90,
        alphaRoughness,
        diffuseColor,
        specularColor);

    vec3 F = specularReflection(pbrInputs);
    float G = geometricOcclusion(pbrInputs);
    float D = microfacetDistribution(pbrInputs);

    diff = (1.0 - F) * diffuse_brdf(pbrInputs);
    spec = F * G * D / (4.0 * NdotL * NdotV);
    nl = NdotL;
}

void main()
{
    vec3 final_color = vec3(0.0);
    vec2 tc = getScreenCoord(vary_fragcoord);
    vec3 pos = getPosition(tc).xyz;

    vec3 lv;
    vec4 proj_tc;
    float dist;
    float l_dist;
    if (clipProjectedLightVars(trans_center, pos, dist, l_dist, lv, proj_tc))
    {
        discard;
    }

    float shadow = 1.0;
    if (u.shadow_indices.x >= 0)
    {
        vec4 shd = texture(lightMap, tc);
        shadow = (u.shadow_indices.x == 0) ? shd.b : shd.a;
        shadow += u.near_far_sun_shadow.w;
        shadow = clamp(shadow, 0.0, 1.0);
    }

    GBufferInfo gb = getGBuffer(tc);
    vec3 n = gb.normal;

    float dist_atten = calcLegacyDistanceAttenuation(dist, u.color_falloff.a);
    if (dist_atten <= 0.0)
    {
        discard;
    }

    lv = u.proj_origin_size.xyz - pos.xyz;
    vec3 h;
    vec3 l;
    vec3 v = -normalize(pos);
    float nh;
    float nl;
    float nv;
    float vh;
    float lightDist;
    calcHalfVectors(lv, n, v, h, l, nh, nl, nv, vh, lightDist);

    vec3 diffuse = gb.albedo.rgb;
    vec4 spec = gb.specular;
    vec3 dlit = vec3(0.0);
    vec3 amb_rgb = vec3(0.0);

    if (get_gbuffer_flag(gb.gbufferFlag, GBUFFER_FLAG_HAS_PBR))
    {
        vec3 orm = spec.rgb;
        float perceptualRoughness = orm.g;
        float metallic = orm.b;
        vec3 f0 = vec3(0.04);
        vec3 baseColor = diffuse.rgb;

        vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
        diffuseColor *= 1.0 - metallic;

        vec3 specularColor = mix(f0, baseColor.rgb, metallic);
        vec3 diffPunc = vec3(0.0);
        vec3 specPunc = vec3(0.0);

        if (proj_tc.x > 0.0 && proj_tc.x < 1.0 &&
            proj_tc.y > 0.0 && proj_tc.y < 1.0)
        {
            float lit = 0.0;
            float amb_da = 0.0;
            lv = normalize(lv);

            if (nl > 0.0)
            {
                amb_da += (nl * 0.5 + 0.5) * u.proj_lod_range_ambiance.w;
                dlit = getProjectedLightDiffuseColor(l_dist, proj_tc.xy);
                vec3 intensity = dist_atten * dlit * 3.25 * shadow;

                pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic,
                            n.xyz, v, normalize(lv), nl, diffPunc, specPunc);

                final_color += intensity * clamp(nl * (diffPunc + specPunc), vec3(0.0), vec3(10.0));
            }

            amb_rgb = getProjectedLightAmbiance(amb_da, dist_atten, lit, nl, 1.0, proj_tc.xy) * 3.25;
            pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic,
                        n.xyz, v, normalize(lv), nl, diffPunc, specPunc);
            final_color += amb_rgb * clamp(nl * (diffPunc + specPunc), vec3(0.0), vec3(10.0));
        }
    }
    else
    {
        float envIntensity = gb.envIntensity;
        diffuse = srgb_to_linear(diffuse);
        spec.rgb = srgb_to_linear(spec.rgb);

        if (proj_tc.z > 0.0 &&
            proj_tc.x < 1.0 &&
            proj_tc.y < 1.0 &&
            proj_tc.x > 0.0 &&
            proj_tc.y > 0.0)
        {
            float amb_da = 0.0;
            float lit = 0.0;

            if (nl > 0.0)
            {
                lit = nl * dist_atten;
                dlit = getProjectedLightDiffuseColor(l_dist, proj_tc.xy);
                final_color = dlit * lit * diffuse * shadow;
                amb_da += (nl * 0.5 + 0.5) * u.proj_lod_range_ambiance.w;
            }

            amb_rgb = getProjectedLightAmbiance(amb_da, dist_atten, lit, nl, 1.0, proj_tc.xy);
            final_color += diffuse.rgb * amb_rgb * max(dot(-normalize(lv), n), 0.0);
        }

        if (spec.a > 0.0)
        {
            dlit *= min(nl * 6.0, 1.0) * dist_atten;
            float fres = pow(1.0 - vh, 5.0) * 0.4 + 0.5;
            float gtdenom = 2.0 * nh;
            float gt = max(0.0, min(gtdenom * nv / vh, gtdenom * nl / vh));

            if (nh > 0.0)
            {
                float scol = fres * texture(lightFunc, vec2(nh, spec.a)).r * gt / (nh * nl);
                vec3 speccol = dlit * scol * spec.rgb * shadow;
                speccol = clamp(speccol, vec3(0.0), vec3(1.0));
                final_color += speccol;
            }
        }

        if (envIntensity > 0.0)
        {
            vec3 ref = reflect(normalize(pos), n);
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
                        final_color += u.color_falloff.rgb *
                            texture2DLodSpecular(stc.xy, (1.0 - spec.a) * (u.proj_lod_range_ambiance.x * 0.6)).rgb *
                            shadow * envIntensity;
                    }
                }
            }
        }
    }

    final_color = max(final_color, vec3(0.0));
    float final_scale = 1.0;
    if (u.screen_res_classic.z > 0.5)
    {
        final_scale = 0.9;
    }

    frag_color.rgb = final_color * final_scale;
    frag_color.a = 0.0;
}
