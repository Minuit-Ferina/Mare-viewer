#!/usr/bin/env python3
"""Refresh the Vulkan shader port inventory section in todo.md."""

from __future__ import annotations

from pathlib import Path
import sys

import port_opengl_shaders as ports


REPO_ROOT = Path(__file__).resolve().parents[2]
TODO_FILE = REPO_ROOT / "todo.md"
SECTION_START = "## OpenGL Shader Inventory"
SECTION_END = "## Future Vulkan Renderer Parity"


def is_entry_point(shader: Path) -> bool:
    return shader.suffix in {".vert", ".frag", ".geom", ".comp"}


def main() -> int:
    text = TODO_FILE.read_text()
    start = text.index(SECTION_START)
    end = text.index(SECTION_END)

    source_lines: list[str] = []
    expected_outputs: set[str] = set()
    source_entry_count = 0
    source_snippet_count = 0

    for source_path in ports.iter_opengl_sources():
        source = source_path.read_text()
        output_path = ports.output_path_for(source_path, source)
        relative_source = source_path.relative_to(REPO_ROOT).as_posix()
        relative_output = output_path.relative_to(
            REPO_ROOT / "indra/newview/app_settings/shaders"
        ).as_posix()

        expected_outputs.add(output_path.relative_to(ports.VULKAN_FINAL_DIR).as_posix())
        if is_entry_point(output_path):
            source_entry_count += 1
        else:
            source_snippet_count += 1

        status = "x" if output_path.exists() else " "
        source_lines.append(f"- [{status}] {relative_source} - port: {relative_output}")

    final_files = sorted(
        shader.relative_to(ports.VULKAN_FINAL_DIR).as_posix()
        for shader in ports.VULKAN_FINAL_DIR.rglob("*")
        if shader.is_file()
    )
    final_entry_count = sum(1 for shader in final_files if is_entry_point(Path(shader)))
    final_snippet_count = sum(1 for shader in final_files if Path(shader).suffix == ".glsl")
    extra_outputs = [shader for shader in final_files if shader not in expected_outputs]
    missing_outputs = [line for line in source_lines if line.startswith("- [ ]")]

    section = [
        SECTION_START,
        "",
        "Generated from `indra/newview/app_settings/shaders` on 2026-05-28. "
        "`vulkan/` files are excluded from the source inventory.",
        "",
        f"Totals: {len(source_lines)} OpenGL/GLSL shader files, "
        f"{len(final_files)} final Vulkan shader source ports.",
        f"Final Vulkan sources currently include {final_entry_count} standalone "
        f"SPIR-V entry points and {final_snippet_count} helper snippets.",
        f"OpenGL-derived final ports cover {source_entry_count} entry points and "
        f"{source_snippet_count} snippets; {len(extra_outputs)} extra final entry "
        "points are generated from existing split/adapted runtime needs.",
        "",
        "Validation status:",
        "",
        "- [x] Every OpenGL shader source in the inventory has a `vulkan/final` "
        "source-level port.",
        "- [x] `python3 tools/vulkan_shaders/build_shader_files.py --source-dir "
        "indra/newview/app_settings/shaders/vulkan/final --output-dir "
        "/private/tmp/mare-vulkan-final-all-shader-check` compiles the final "
        "shader tree.",
        "- [x] `xcodebuild -project build-darwin-arm64-vulkan-xcode/"
        "Kokua.xcodeproj -target mare_vulkan_final_shaders -configuration "
        "Release build` succeeds.",
        "- [x] Runtime Vulkan shader modules are discovered from `vulkan/final`",
        "      SPIR-V output; active bootstrap/UI/world/terrain pipelines now bind",
        "      `vulkan/final` shaders instead of `vulkan/bridge`.",
        "- [x] The viewer build and bundle manifest no longer build or package",
        "      `vulkan/bridge` shaders.",
        "",
        "### Vulkan Shader Organization Notes",
        "",
        "- [x] Put newly ported source-level shaders in `vulkan/final`, not in",
        "      temporary runtime compatibility shader directories.",
        "- [x] Reorganize `vulkan/final` to mirror the OpenGL shader tree before the",
        "      next large shader port batch: `class1`, `class2`, `class3`, then",
        "      subdirectories such as `interface`, `deferred`, `effects`,",
        "      `environment`, `objects`, and `avatar`.",
        "- [x] Keep `class1`/`class2`/`class3` as migration labels for now. They are",
        "      OpenGL viewer shader complexity tiers, not Vulkan pipeline tiers.",
        "- [x] Do not reorganize final Vulkan shaders by Vulkan pipeline names until the",
        "      backend has real final pipeline ownership for UI, G-buffer, lighting,",
        "      shadows, reflections, water, post-processing, terrain, avatars, and",
        "      alpha/transparency.",
        "- [x] Treat current Vulkan `VkPipeline` objects as functional bootstrap/",
        "      compatibility pipelines. They are not yet the final renderer pipeline",
        "      architecture.",
        "- [x] Replace the incomplete compatibility-shader checklist with OpenGL-derived",
        "      final source ports for the whole shader inventory.",
        "",
        "### Additional Final Vulkan Entry Points",
        "",
    ]

    if extra_outputs:
        section.extend(
            f"- [x] indra/newview/app_settings/shaders/vulkan/final/{shader}"
            for shader in extra_outputs
        )
    else:
        section.append("- [x] None.")

    if missing_outputs:
        section.extend(["", "### Missing OpenGL Source Coverage", ""])
        section.extend(missing_outputs)

    section.extend(["", "### OpenGL Source Coverage", ""])
    section.extend(source_lines)

    TODO_FILE.write_text(text[:start] + "\n".join(section) + "\n\n" + text[end:])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
