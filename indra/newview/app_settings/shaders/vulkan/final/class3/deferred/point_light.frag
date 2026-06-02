#version 450

// Faithful source-level Vulkan port of class3/deferred/pointLightF.glsl.
// Runtime descriptor population for this final shader is wired separately.

layout(set = 0, binding = 0) uniform sampler2D diffuseRect;
layout(set = 0, binding = 1) uniform sampler2D specularRect;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D depthMap;
layout(set = 0, binding = 4) uniform sampler2D lightFunc;

layout(set = 1, binding = 0) uniform MarePointLightFragmentUniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    vec4 center_size;        // xyz = center, w = size
    mat4 inv_proj;
    vec4 color_falloff;       // rgb = color, a = falloff
    vec4 size_classic;        // x = size, y = classic_mode
    vec4 screen_res_sun_wash; // xy = screen_res, z = sun_wash
    vec4 viewport;
    vec4 env_mat0;
    vec4 env_mat1;
    vec4 env_mat2;
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

vec4 getNorm(vec2 screenpos)
{
    return decodeNormal(getNormRaw(screenpos));
}

GBufferInfo getGBuffer(vec2 screenpos)
{
    GBufferInfo ret;
    vec4 normInfo = getNormRaw(screenpos);
    ret.albedo = texture(diffuseRect, screenpos.xy);
    ret.normal = decodeNormal(normInfo).xyz;
    ret.specular = texture(specularRect, screenpos.xy);
    ret.envIntensity = normInfo.b;
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
    GBufferInfo gb = getGBuffer(tc);

    vec3 n = gb.normal;
    vec3 diffuse = gb.albedo.rgb;
    vec4 spec = gb.specular;

    vec3 lv = trans_center.xyz - pos;
    vec3 h;
    vec3 l;
    vec3 v = -normalize(pos);
    float nh;
    float nl;
    float nv;
    float vh;
    float lightDist;
    calcHalfVectors(lv, n, v, h, l, nh, nl, nv, vh, lightDist);

    float size = u.size_classic.x;
    float falloff = u.color_falloff.a;
    if (lightDist >= size)
    {
        discard;
    }

    float dist = lightDist / size;
    float dist_atten = calcLegacyDistanceAttenuation(dist, falloff);
    vec3 light_color = u.color_falloff.rgb;

    if (spec.a > 0.5 || get_gbuffer_flag(gb.gbufferFlag, GBUFFER_FLAG_HAS_PBR))
    {
        vec3 orm = spec.rgb;
        float perceptualRoughness = orm.g;
        float metallic = orm.b;
        vec3 f0 = vec3(0.04);
        vec3 baseColor = diffuse.rgb;
        vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
        diffuseColor *= 1.0 - metallic;
        vec3 specularColor = mix(f0, baseColor.rgb, metallic);
        vec3 intensity = dist_atten * light_color * 3.25;

        float pbr_nl = 0.0;
        vec3 diffPunc = vec3(0.0);
        vec3 specPunc = vec3(0.0);
        pbrPunctual(diffuseColor, specularColor, perceptualRoughness, metallic,
                    n.xyz, v, normalize(lv), pbr_nl, diffPunc, specPunc);

        final_color += intensity * clamp(pbr_nl * (diffPunc + specPunc), vec3(0.0), vec3(10.0));
    }
    else
    {
        if (nl < 0.0)
        {
            discard;
        }

        diffuse = srgb_to_linear(diffuse);
        spec.rgb = srgb_to_linear(spec.rgb);

        float lit = nl * dist_atten;
        final_color = light_color.rgb * lit * diffuse;

        if (spec.a > 0.0)
        {
            lit = min(nl * 6.0, 1.0) * dist_atten;
            float fres = pow(1.0 - vh, 5.0) * 0.4 + 0.5;
            float gtdenom = 2.0 * nh;
            float gt = max(0.0, min(gtdenom * nv / vh, gtdenom * nl / vh));

            if (nh > 0.0)
            {
                float scol = fres * texture(lightFunc, vec2(nh, spec.a)).r * gt / (nh * nl);
                final_color += lit * scol * light_color.rgb * spec.rgb;
            }
        }

        if (dot(final_color, final_color) <= 0.0)
        {
            discard;
        }
    }

    float final_scale = 1.0;
    if (u.size_classic.y > 0.5)
    {
        final_scale = 0.9;
    }

    frag_color.rgb = max(final_color * final_scale, vec3(0.0));
    frag_color.a = 0.0;
}
