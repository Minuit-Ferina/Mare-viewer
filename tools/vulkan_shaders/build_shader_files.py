#!/usr/bin/env python3
"""Compile Mare Vulkan GLSL shaders into standalone SPIR-V files."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SHADER_DIR = REPO_ROOT / "indra/newview/app_settings/shaders/vulkan/final"

SHADER_SUFFIXES = {".vert", ".frag", ".geom", ".comp"}


def find_glslang(explicit: str | None) -> str:
    if explicit:
        return explicit

    vulkan_sdk = os.environ.get("VULKAN_SDK")
    if vulkan_sdk:
        candidate = Path(vulkan_sdk) / "bin/glslangValidator"
        if candidate.exists():
            return str(candidate)

    home = Path.home()
    if home:
        for candidate in sorted(home.glob("VulkanSDK/*/macOS/bin/glslangValidator"), reverse=True):
            if candidate.exists():
                return str(candidate)

    resolved = shutil.which("glslangValidator")
    if resolved:
        return resolved

    raise SystemExit("glslangValidator not found. Pass --glslang or set VULKAN_SDK.")


def compile_shader(glslang: str, source: Path, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [glslang, "-V", str(source), "-o", str(output)],
        cwd=REPO_ROOT,
        check=True,
    )


def iter_shader_sources(source_dir: Path) -> list[Path]:
    return sorted(
        source
        for source in source_dir.rglob("*")
        if source.is_file() and source.suffix in SHADER_SUFFIXES
    )


def shader_output_path(source_dir: Path, output_dir: Path, shader_source: Path) -> Path:
    relative_source = shader_source.relative_to(source_dir)
    return output_dir / relative_source.parent / f"{relative_source.name}.spv"


def remove_stale_spirv(output_dir: Path, expected_outputs: set[Path]) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    for output in output_dir.rglob("*.spv"):
        if output.relative_to(output_dir) not in expected_outputs:
            output.unlink()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--glslang",
        help="Path to glslangValidator. Defaults to $VULKAN_SDK/bin/glslangValidator, ~/VulkanSDK, or PATH.",
    )
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=DEFAULT_SHADER_DIR,
        help="Directory containing Vulkan GLSL sources.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        required=True,
        help="Directory for generated .spv files.",
    )
    args = parser.parse_args()
    if args.output_dir.resolve() == args.source_dir.resolve():
        raise SystemExit("Refusing to write generated .spv files into the source shader directory.")

    glslang = find_glslang(args.glslang)
    shader_sources = iter_shader_sources(args.source_dir)
    if not shader_sources:
        raise SystemExit(f"No Vulkan shader sources found in {args.source_dir}.")

    remove_stale_spirv(
        args.output_dir,
        {
            shader_output_path(args.source_dir, args.output_dir, shader_source).relative_to(args.output_dir)
            for shader_source in shader_sources
        },
    )

    for shader_source in shader_sources:
        compile_shader(
            glslang,
            shader_source,
            shader_output_path(args.source_dir, args.output_dir, shader_source),
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
