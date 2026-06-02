#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 1) uniform sampler2D specularOrOrmMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D emissiveMap;
layout(set = 0, binding = 4) uniform sampler2D depthMap;
layout(set = 0, binding = 5) uniform sampler2D lightMap;
layout(set = 0, binding = 6) uniform samplerCube environmentMap;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;

layout(location = 0) out vec4 frag_color;

layout(push_constant) uniform MareWorldPushConstants
{
    layout(offset = 80) vec4 composite_ambient;
    layout(offset = 112) vec4 composite_sun_direction;
    layout(offset = 128) vec4 composite_light;
    layout(offset = 144) vec4 composite_moon_direction;
    layout(offset = 160) vec4 composite_sky_settings;
    layout(offset = 176) vec4 composite_features;
    layout(offset = 192) vec4 composite_light_direction;
    layout(offset = 208) vec4 composite_local_light;
    layout(offset = 224) vec4 composite_ssao;
    layout(offset = 272) vec4 composite_environment0;
    layout(offset = 288) vec4 composite_environment1;
    layout(offset = 304) vec4 composite_environment2;
    layout(offset = 416) vec4 scene_reflection;
} pc;

vec3 srgb_to_linear(vec3 color)
{
    bvec3 cutoff = lessThanEqual(color, vec3(0.04045));
    vec3 low = color / 12.92;
    vec3 high = pow((color + vec3(0.055)) / 1.055, vec3(2.4));
    return mix(high, low, cutoff);
}

vec3 decode_gbuffer_normal(vec4 encoded)
{
    vec3 normal = normalize(encoded.xyz * 2.0 - 1.0);
    if (dot(normal, normal) <= 0.0001)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    return normal;
}

vec3 fallback_sky_color(vec2 texcoord)
{
    float horizon = smoothstep(0.0, 1.0, texcoord.y);
    vec3 ambient_color = max(pc.composite_ambient.rgb, vec3(0.0));
    vec3 direct_color = max(pc.composite_light.rgb, vec3(0.0));
    float sky_hdr_scale =
        pc.scene_reflection.w > 0.5 ?
            max(pc.composite_sky_settings.y, 1.0) :
            1.0;
    vec3 horizon_color = ambient_color * 1.15 + direct_color * 0.08;
    vec3 zenith_color = ambient_color * 0.72 + direct_color * 0.24;
    return max(mix(horizon_color, zenith_color, horizon) * sky_hdr_scale, vec3(0.0));
}

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 approximate_view_direction(vec2 texcoord)
{
    vec2 dimensions = max(vec2(textureSize(diffuseMap, 0)), vec2(1.0));
    float aspect = dimensions.x / dimensions.y;
    vec2 ndc = texcoord * 2.0 - 1.0;
    ndc.y = -ndc.y;
    ndc.x *= aspect;
    return normalize(vec3(-ndc, 1.0));
}

vec3 sample_environment(vec3 normal, vec3 view_dir, float roughness, float probe_ambiance)
{
    if (pc.scene_reflection.y < 0.5 || probe_ambiance <= 0.0)
    {
        return vec3(0.0);
    }

    vec3 reflected = reflect(-view_dir, normal);
    vec3 env_dir = mat3(
        pc.composite_environment0.xyz,
        pc.composite_environment1.xyz,
        pc.composite_environment2.xyz) * reflected;
    vec3 cube_color = textureLod(
        environmentMap,
        normalize(env_dir),
        clamp(roughness * pc.composite_sky_settings.w, 0.0, pc.composite_sky_settings.w)).rgb;
    return max(cube_color, vec3(0.0)) * max(probe_ambiance, 0.0);
}

vec3 select_composite_light_direction()
{
    vec3 selected_light_dir =
        pc.composite_sun_direction.w > 0.5 ?
            pc.composite_sun_direction.xyz :
            pc.composite_moon_direction.xyz;
    if (dot(selected_light_dir, selected_light_dir) <= 0.0001)
    {
        selected_light_dir = pc.composite_light_direction.xyz;
    }
    if (dot(selected_light_dir, selected_light_dir) <= 0.0001)
    {
        selected_light_dir = vec3(0.32, 0.48, 0.82);
    }
    return normalize(selected_light_dir);
}

float compute_fallback_ssao(vec2 texcoord, float enabled)
{
    if (enabled < 0.5)
    {
        return 1.0;
    }

    float center_depth = texture(depthMap, texcoord).r;
    if (center_depth >= 0.99999)
    {
        return 1.0;
    }

    vec2 texel = 1.0 / max(vec2(textureSize(depthMap, 0)), vec2(1.0));
    float radius = clamp(pc.composite_ssao.x, 0.5, max(pc.composite_ssao.y, 1.0));
    float factor = clamp(pc.composite_ssao.z, 0.1, 8.0);
    float strength = clamp(pc.composite_ssao.w, 0.0, 2.0);
    vec2 sample_offset = texel * radius;

    float occlusion = 0.0;
    float sample_depth = texture(depthMap, texcoord + vec2(sample_offset.x, 0.0)).r;
    occlusion += clamp((center_depth - sample_depth) * factor * 64.0, 0.0, 1.0);
    sample_depth = texture(depthMap, texcoord - vec2(sample_offset.x, 0.0)).r;
    occlusion += clamp((center_depth - sample_depth) * factor * 64.0, 0.0, 1.0);
    sample_depth = texture(depthMap, texcoord + vec2(0.0, sample_offset.y)).r;
    occlusion += clamp((center_depth - sample_depth) * factor * 64.0, 0.0, 1.0);
    sample_depth = texture(depthMap, texcoord - vec2(0.0, sample_offset.y)).r;
    occlusion += clamp((center_depth - sample_depth) * factor * 64.0, 0.0, 1.0);

    return clamp(1.0 - occlusion * 0.25 * strength, 0.25, 1.0);
}

float local_light_screen_weight(vec2 texcoord, vec2 light_center, float light_radius)
{
    if (light_radius <= 0.0 ||
        light_center.x < 0.0 ||
        light_center.y < 0.0)
    {
        return 1.0;
    }

    float distance_from_light = distance(texcoord, clamp(light_center, vec2(0.0), vec2(1.0)));
    return smoothstep(light_radius, light_radius * 0.25, distance_from_light);
}

void main()
{
    vec2 tc = vary_texcoord0.xy;
    vec4 diffuse = texture(diffuseMap, tc);
    vec4 specular_or_orm = texture(specularOrOrmMap, tc);
    vec4 encoded_normal = texture(normalMap, tc);
    vec3 emissive = pc.composite_features.x > 3.5 ?
        max(texture(emissiveMap, tc).rgb, vec3(0.0)) :
        vec3(0.0);
    float scene_depth = texture(depthMap, tc).r;

    if (encoded_normal.a < 0.5 || scene_depth >= 0.99999)
    {
        vec3 sky_or_color = max(diffuse.rgb, vec3(0.0));
        vec3 sky_fallback = fallback_sky_color(tc);
        if (scene_depth >= 0.99999)
        {
            sky_or_color = mix(sky_or_color, sky_fallback, 0.7);
        }
        if (max(max(sky_or_color.r, sky_or_color.g), sky_or_color.b) < 0.002)
        {
            sky_or_color = sky_fallback;
        }
        frag_color = vec4((sky_or_color + emissive) * vertex_color.rgb, 1.0);
        return;
    }

    vec2 shadow_ao = texture(lightMap, tc).rg;
    float sun_shadow = max(shadow_ao.r, diffuse.a);
    float ambient_occlusion = clamp(shadow_ao.g, 0.0, 1.0);
    if (shadow_ao.r <= 0.0001 && shadow_ao.g <= 0.0001)
    {
        sun_shadow = 1.0;
        ambient_occlusion = compute_fallback_ssao(tc, pc.composite_features.y);
    }

    vec3 normal = decode_gbuffer_normal(encoded_normal);
    vec3 light_dir = select_composite_light_direction();
    float ndotl = max(dot(normal, light_dir), 0.0);
    bool classic_mode = pc.composite_moon_direction.w > 0.5;
    if (classic_mode)
    {
        ndotl = pow(ndotl, 1.2);
    }

    bool pbr = specular_or_orm.a > 0.5;
    vec3 base_color = max(diffuse.rgb, vec3(0.0));
    if (!pbr)
    {
        base_color = srgb_to_linear(base_color);
        specular_or_orm.rgb = srgb_to_linear(max(specular_or_orm.rgb, vec3(0.0)));
    }

    float env = clamp(diffuse.a, 0.0, 1.0);
    float ao = pbr ? clamp(specular_or_orm.r, 0.0, 1.0) : 1.0;
    float legacy_shiny = clamp(specular_or_orm.a * 2.0, 0.0, 1.0);
    float roughness = pbr ?
        clamp(specular_or_orm.g, 0.04, 1.0) :
        mix(0.72, 0.22, legacy_shiny);
    float metallic = pbr ? clamp(specular_or_orm.b, 0.0, 1.0) : 0.0;
    float combined_ao = min(ao, ambient_occlusion);

    float probe_ambiance = clamp(pc.scene_reflection.x, 0.0, 1.0);
    vec3 ambient_color = max(pc.composite_ambient.rgb, vec3(0.02));
    ambient_color = mix(
        ambient_color,
        max(ambient_color, vec3(probe_ambiance * 0.25)),
        probe_ambiance);
    vec3 direct_color = max(pc.composite_light.rgb, vec3(0.0));
    float direct_scale = max(pc.composite_light_direction.a, 0.0);
    if (classic_mode)
    {
        direct_scale *= 1.35;
    }

    vec3 view_dir = approximate_view_direction(tc);
    vec3 sampled_environment =
        sample_environment(normal, view_dir, roughness, probe_ambiance);
    float environment_scale = mix(1.0, 1.75, probe_ambiance);
    vec3 fallback_environment =
        base_color * env * environment_scale * mix(0.08, 0.18, 1.0 - roughness);
    vec3 environment = mix(
        fallback_environment,
        sampled_environment * env,
        clamp(probe_ambiance, 0.0, 1.0));
    vec3 ambient = base_color * ambient_color * combined_ao + environment * combined_ao;
    vec3 direct = base_color * direct_color * ndotl * direct_scale * sun_shadow * mix(1.0, 0.62, metallic);

    vec3 local_light_color = max(pc.composite_local_light.rgb, vec3(0.0));
    float local_light_strength =
        clamp(pc.composite_local_light.a, 0.0, 1.0) *
        local_light_screen_weight(
            tc,
            pc.composite_features.zw,
            pc.composite_light.a);
    vec3 local_light = base_color * local_light_color * local_light_strength *
        combined_ao * mix(0.55, 1.0, 1.0 - roughness);

    vec3 half_dir = normalize(light_dir + view_dir);
    vec3 specular_color = pbr ?
        mix(vec3(0.04), base_color, metallic) :
        max(specular_or_orm.rgb, vec3(0.0));
    float specular_power = mix(128.0, 8.0, roughness);
    float ndoth = max(dot(normal, half_dir), 0.0);
    vec3 fresnel = pbr ?
        fresnel_schlick(max(dot(half_dir, view_dir), 0.0), specular_color) :
        specular_color * mix(0.35, 1.2, legacy_shiny);
    float specular = pow(ndoth, specular_power) * (1.0 - roughness);
    vec3 direct_specular = fresnel * direct_color * specular * direct_scale * sun_shadow;
    vec3 local_specular = fresnel * local_light_color * local_light_strength *
        specular * mix(0.35, 0.75, 1.0 - roughness);

    vec3 color = ambient + direct + local_light + direct_specular + local_specular + emissive;
    frag_color = vec4(color * vertex_color.rgb, 1.0);
}
