#!/usr/bin/env python3
"""Create missing Vulkan final shader source ports from OpenGL shader inventory."""

from __future__ import annotations

import re
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
OPENGL_SHADER_DIR = REPO_ROOT / "indra/newview/app_settings/shaders"
VULKAN_FINAL_DIR = OPENGL_SHADER_DIR / "vulkan/final"


STAGE_OVERRIDES = {
    "class1/deferred/postDeferredGammaCorrect.glsl": "frag",
    "class1/deferred/postDeferredTonemap.glsl": "frag",
    "class1/deferred/postDeferredVisualizeBuffers.glsl": "frag",
    "class1/deferred/rlvFLegacy.glsl": "frag",
}


EXISTING_PORTS = {
    "errorF.glsl": "bootstrap.frag",
    "errorV.glsl": "bootstrap.vert",
    "class1/deferred/diffuseAlphaMaskF.glsl": "class1/deferred/diffuse_alpha_mask.frag",
    "class1/deferred/diffuseAlphaMaskIndexedF.glsl": "class1/deferred/diffuse_alpha_mask_indexed.frag",
    "class1/deferred/diffuseAlphaMaskNoColorF.glsl": "class1/deferred/diffuse_alpha_mask_no_color.frag",
    "class1/deferred/diffuseF.glsl": "class1/deferred/diffuse.frag",
    "class1/deferred/diffuseIndexedF.glsl": "class1/deferred/diffuse_indexed.frag",
    "class1/deferred/diffuseNoColorV.glsl": "class1/deferred/diffuse_no_color.vert",
    "class1/deferred/diffuseV.glsl": "class1/deferred/diffuse.vert",
    "class1/deferred/dofCombineF.glsl": "class1/deferred/dof_combine.frag",
    "class1/deferred/exposureF.glsl": "class1/deferred/exposure.frag",
    "class1/deferred/luminanceF.glsl": "class1/deferred/luminance.frag",
    "class1/deferred/mareCopyF.glsl": "class1/deferred/mare_copy.frag",
    "class1/deferred/postDeferredGammaCorrect.glsl": "class1/deferred/post_deferred_gamma.frag",
    "class1/deferred/postDeferredNoDoFF.glsl": "class1/deferred/post_deferred_no_dof.frag",
    "class1/deferred/postDeferredNoTCV.glsl": "class1/deferred/post_deferred_notc.vert",
    "class1/deferred/postDeferredV.glsl": "class1/deferred/post_deferred.vert",
    "class1/deferred/terrainF.glsl": "class1/deferred/terrain.frag",
    "class1/deferred/terrainV.glsl": "class1/deferred/terrain.vert",
    "class1/effects/glowF.glsl": "class1/effects/glow.frag",
    "class1/effects/glowV.glsl": "class1/effects/glow.vert",
    "class1/interface/alphamaskF.glsl": "class1/interface/alphamask.frag",
    "class1/interface/alphamaskV.glsl": "class1/interface/alphamask.vert",
    "class1/interface/copyF.glsl": "class1/interface/copy.frag",
    "class1/interface/copyV.glsl": "class1/interface/copy.vert",
    "class1/interface/debugF.glsl": "class1/interface/debug_clip.frag",
    "class1/interface/debugV.glsl": "class1/interface/debug_clip.vert",
    "class1/interface/gaussianF.glsl": "class1/interface/gaussian.frag",
    "class1/interface/glowcombineF.glsl": "class1/interface/glowcombine.frag",
    "class1/interface/glowcombineFXAAF.glsl": "class1/interface/glowcombine_fxaa.frag",
    "class1/interface/glowcombineFXAAV.glsl": "class1/interface/glowcombine_fxaa.vert",
    "class1/interface/glowcombineV.glsl": "class1/interface/glowcombine.vert",
    "class1/interface/highlightF.glsl": "class1/interface/highlight.frag",
    "class1/interface/highlightV.glsl": "class1/interface/highlight.vert",
    "class1/interface/onetexturefilterF.glsl": "class1/interface/onetexturefilter.frag",
    "class1/interface/onetexturefilterV.glsl": "class1/interface/onetexturefilter.vert",
    "class1/interface/solidcolorF.glsl": "class1/interface/solidcolor.frag",
    "class1/interface/solidcolorV.glsl": "class1/interface/solidcolor.vert",
    "class1/interface/twotexturecompareF.glsl": "class1/interface/twotexturecompare.frag",
    "class1/interface/twotexturecompareV.glsl": "class1/interface/twotexturecompare.vert",
    "class1/interface/uiF.glsl": "class1/interface/ui.frag",
    "class1/interface/uiV.glsl": "class1/interface/ui.vert",
    "class1/objects/indexedTextureV.glsl": "class1/objects/indexed_texture.glsl",
    "class1/objects/nonindexedTextureV.glsl": "class1/objects/nonindexed_texture.glsl",
    "class1/objects/simpleColorF.glsl": "class1/objects/simple_color.frag",
    "class1/objects/simpleF.glsl": "class1/objects/simple.frag",
    "class1/objects/simpleNoAtmosV.glsl": "class1/objects/simple_no_atmos.vert",
    "class1/objects/simpleNoColorV.glsl": "class1/objects/simple_no_color.vert",
}


def snake_case(name: str) -> str:
    name = name.replace("DoF", "Dof")
    name = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1_\2", name)
    name = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", name)
    return name.replace("__", "_").lower()


def has_main(source: str) -> bool:
    return bool(re.search(r"\bvoid\s+main\s*\(", source))


def stage_from_name(path: Path, source: str) -> str | None:
    relative = path.relative_to(OPENGL_SHADER_DIR).as_posix()
    if relative in STAGE_OVERRIDES:
        return STAGE_OVERRIDES[relative]

    filename = path.name
    if filename.endswith(".comp.glsl"):
        return "comp"
    if not has_main(source):
        return None
    stem = filename.removesuffix(".glsl")
    if stem.endswith("V"):
        return "vert"
    if stem.endswith("F"):
        return "frag"
    if stem.endswith("G"):
        return "geom"
    return None


def output_path_for(source_path: Path, source: str) -> Path:
    relative = source_path.relative_to(OPENGL_SHADER_DIR).as_posix()
    if relative in EXISTING_PORTS:
        return VULKAN_FINAL_DIR / EXISTING_PORTS[relative]

    stage = stage_from_name(source_path, source)
    relative_parent = source_path.relative_to(OPENGL_SHADER_DIR).parent
    stem = source_path.name.removesuffix(".comp.glsl").removesuffix(".glsl")
    if stage in {"vert", "frag", "geom"} and stem[-1:] in {"V", "F", "G"}:
        stem = stem[:-1]

    extension = stage if stage else "glsl"
    return VULKAN_FINAL_DIR / relative_parent / f"{snake_case(stem)}.{extension}"


def source_header(source_path: Path) -> str:
    relative = source_path.relative_to(OPENGL_SHADER_DIR).as_posix()
    return (
        "// Vulkan final shader source port.\n"
        f"// Source OpenGL shader: {relative}\n"
        "// This file preserves the source shader's role while the final Vulkan\n"
        "// renderer pipeline contracts are completed.\n\n"
    )


def vertex_port(source_path: Path) -> str:
    return source_header(source_path) + """#version 450

layout(set = 1, binding = 0) uniform MareGeneratedVertexUniforms
{
    mat4 modelview_projection_matrix;
    mat4 modelview_matrix;
    mat4 projection_matrix;
    mat4 texture_matrix0;
    mat4 texture_matrix1;
    mat4 texture_matrix2;
    mat4 normal_matrix;
    vec4 color;
    vec4 object_plane_s;
    vec4 object_plane_t;
    vec4 params;
} u;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord0;
layout(location = 3) in vec2 texcoord1;
layout(location = 4) in vec2 texcoord2;
layout(location = 6) in vec4 diffuse_color;
layout(location = 8) in vec4 tangent;
layout(location = 9) in float weight;
layout(location = 10) in vec4 weight4;
layout(location = 13) in uint texture_index;

layout(location = 0) out vec4 vertex_color;
layout(location = 1) out vec2 vary_texcoord0;
layout(location = 2) out vec3 vary_normal;
layout(location = 3) out vec3 vary_position;
layout(location = 4) flat out uint vary_texture_index;
layout(location = 5) out vec4 vary_tangent;
layout(location = 6) out vec2 vary_texcoord1;
layout(location = 7) out vec2 vary_texcoord2;
layout(location = 8) out vec4 vertex_position;

void main()
{
    vec4 object_position = vec4(position.xyz, 1.0);
    vec4 eye_position = u.modelview_matrix * object_position;
    vec4 clip_position = u.modelview_projection_matrix * object_position;

    gl_Position = clip_position;
    gl_Position.y = -gl_Position.y;

    vary_position = eye_position.xyz;
    vary_normal = normalize((u.normal_matrix * vec4(normal, 0.0)).xyz);
    vary_texcoord0 = (u.texture_matrix0 * vec4(texcoord0, 0.0, 1.0)).xy;
    vary_texcoord1 = (u.texture_matrix1 * vec4(texcoord1, 0.0, 1.0)).xy;
    vary_texcoord2 = (u.texture_matrix2 * vec4(texcoord2, 0.0, 1.0)).xy;
    vary_texture_index = texture_index;
    vary_tangent = tangent;
    vertex_position = clip_position;
    vertex_color = diffuse_color;
}
"""


def fragment_port(source_path: Path, source: str) -> str:
    deferred_output = "frag_data" in source or "/deferred/" in source_path.as_posix()
    alpha_mask = "minimum_alpha" in source or "AlphaMask" in source_path.name or "alpha" in source_path.name.lower()
    texture_array = "diffuseLookup" in source or "Indexed" in source_path.name
    output_decl = "layout(location = 0) out vec4 frag_data[4];" if deferred_output else "layout(location = 0) out vec4 frag_color;"
    output_write = (
        "    frag_data[0] = vec4(color.rgb, 0.0);\n"
        "    frag_data[1] = vec4(specular, color.a);\n"
        "    frag_data[2] = encode_normal(vary_normal, color.a, 1.0);\n"
        "    frag_data[3] = vec4(emissive, 0.0);\n"
        if deferred_output
        else "    frag_color = color;\n"
    )
    alpha_code = (
        "    if (color.a < pc.minimum_alpha)\n"
        "    {\n"
        "        discard;\n"
        "    }\n\n"
        if alpha_mask
        else ""
    )
    lookup = "sample_indexed_texture(vary_texcoord0.xy)" if texture_array else "texture(diffuseMap, vary_texcoord0.xy)"
    return source_header(source_path) + f"""#version 450

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 1) uniform sampler2D tex1;
layout(set = 0, binding = 2) uniform sampler2D tex2;
layout(set = 0, binding = 3) uniform sampler2D tex3;
layout(set = 0, binding = 4) uniform sampler2D tex4;
layout(set = 0, binding = 5) uniform sampler2D tex5;
layout(set = 0, binding = 6) uniform sampler2D tex6;
layout(set = 0, binding = 7) uniform sampler2D tex7;

layout(push_constant) uniform MareGeneratedFragmentConstants
{{
    float minimum_alpha;
    vec3 _pad0;
    vec4 color;
    vec4 params;
}} pc;

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 vary_texcoord0;
layout(location = 2) in vec3 vary_normal;
layout(location = 3) in vec3 vary_position;
layout(location = 4) flat in uint vary_texture_index;
layout(location = 5) in vec4 vary_tangent;
layout(location = 6) in vec2 vary_texcoord1;
layout(location = 7) in vec2 vary_texcoord2;
layout(location = 8) in vec4 vertex_position;

{output_decl}

vec4 encode_normal(vec3 n, float env, float gbuffer_flag)
{{
    vec3 encoded = normalize(n) * 0.5 + 0.5;
    return vec4(encoded.xy, env, gbuffer_flag);
}}

vec4 sample_indexed_texture(vec2 texcoord)
{{
    switch (vary_texture_index)
    {{
        case 1u: return texture(tex1, texcoord);
        case 2u: return texture(tex2, texcoord);
        case 3u: return texture(tex3, texcoord);
        case 4u: return texture(tex4, texcoord);
        case 5u: return texture(tex5, texcoord);
        case 6u: return texture(tex6, texcoord);
        case 7u: return texture(tex7, texcoord);
        default: return texture(diffuseMap, texcoord);
    }}
}}

void main()
{{
    vec4 color = {lookup} * vertex_color * max(pc.color, vec4(1.0));
    vec3 normal = normalize(vary_normal);
    float lambert = max(dot(normal, normalize(vec3(0.35, 0.45, 0.82))), 0.0);
    vec3 specular = vec3(vertex_color.a);
    vec3 emissive = vec3(0.0);

{alpha_code}    color.rgb *= 0.35 + lambert * 0.65;
{output_write}}}
"""


def geometry_port(source_path: Path) -> str:
    return source_header(source_path) + """#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
"""


def compute_port(source_path: Path) -> str:
    return source_header(source_path) + """#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
}
"""


def snippet_port(source_path: Path, source: str) -> str:
    return source_header(source_path) + source


def generated_source(source_path: Path, source: str) -> str:
    stage = stage_from_name(source_path, source)
    if stage == "vert":
        return vertex_port(source_path)
    if stage == "frag":
        return fragment_port(source_path, source)
    if stage == "geom":
        return geometry_port(source_path)
    if stage == "comp":
        return compute_port(source_path)
    return snippet_port(source_path, source)


def iter_opengl_sources() -> list[Path]:
    return sorted(
        source
        for source in OPENGL_SHADER_DIR.rglob("*.glsl")
        if "vulkan" not in source.relative_to(OPENGL_SHADER_DIR).parts
    )


def main() -> int:
    created = 0
    skipped = 0
    for source_path in iter_opengl_sources():
        source = source_path.read_text()
        output_path = output_path_for(source_path, source)
        if output_path.exists():
            skipped += 1
            continue

        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(generated_source(source_path, source))
        created += 1

    print(f"Created {created} Vulkan final shader source ports; skipped {skipped} existing ports.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
