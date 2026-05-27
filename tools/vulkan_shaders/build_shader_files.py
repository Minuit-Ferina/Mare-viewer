#!/usr/bin/env python3
"""Compile Mare Vulkan GLSL bridge shaders into standalone SPIR-V files."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SHADER_DIR = REPO_ROOT / "indra/newview/app_settings/shaders/vulkan/bridge"

SHADERS = [
    "bootstrap.vert",
    "bootstrap.frag",
    "ui.vert",
    "ui.frag",
    "world_textured.vert",
    "world_textured.frag",
    "terrain.vert",
    "terrain.frag",
]


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
        help="Directory containing Vulkan bridge GLSL sources.",
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
    for shader_name in SHADERS:
        compile_shader(
            glslang,
            args.source_dir / shader_name,
            args.output_dir / f"{shader_name}.spv",
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
