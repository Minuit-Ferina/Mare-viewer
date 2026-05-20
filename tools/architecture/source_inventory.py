#!/usr/bin/env python3

from __future__ import annotations

import csv
import re
import sys
from pathlib import Path

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd().resolve()

SOURCE_EXTS = {
    ".c", ".cc", ".cpp", ".cxx",
    ".h", ".hh", ".hpp", ".hxx",
    ".glsl", ".vert", ".frag",
}

SKIP_DIRS = {
    ".git",
    ".venv",
    "build",
    "build-linux-x86_64",
    "build-vc",
    "build-darwin",
    "packages",
    "stage",
    "tmp",
}

PATTERNS = {
    "gl_calls": re.compile(r"\bgl[A-Z][A-Za-z0-9_]*\b"),
    "gGL": re.compile(r"\bgGL\b"),
    "LLGL": re.compile(r"\bLLGL[A-Za-z0-9_]*\b"),
    "LLRender": re.compile(r"\bLLRender\b"),
    "LLPipeline": re.compile(r"\bLLPipeline\b"),
    "LLDrawPool": re.compile(r"\bLLDrawPool[A-Za-z0-9_]*\b"),
    "LLRenderTarget": re.compile(r"\bLLRenderTarget\b"),
    "LLImageGL": re.compile(r"\bLLImageGL\b"),
    "LLViewerTexture": re.compile(r"\bLLViewerTexture\b"),
    "LLViewerWindow": re.compile(r"\bLLViewerWindow\b"),
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+[<"]([^>"]+)[>"]', re.MULTILINE)


def should_skip(path: Path) -> bool:
    parts = set(path.parts)
    return any(part in SKIP_DIRS for part in parts)


def guess_category(rel: str, counts: dict[str, int]) -> str:
    p = rel.lower()

    if "/llrender/" in p:
        return "render.legacy_low_level"
    if "pipeline" in p or counts["LLPipeline"] > 0:
        return "render.pipeline"
    if "lldrawpool" in p or counts["LLDrawPool"] > 0:
        return "render.draw_pool"
    if "llfloater" in p or "llpanel" in p or "/skins/" in p:
        return "ui"
    if "llvoavatar" in p or "avatar" in p:
        return "world.avatar"
    if "texture" in p or counts["LLViewerTexture"] > 0 or counts["LLImageGL"] > 0:
        return "assets.texture"
    if "llviewerwindow" in p:
        return "ui.window"
    if counts["gl_calls"] > 0 or counts["gGL"] > 0 or counts["LLGL"] > 0:
        return "render.opengl_touching"
    if "/newview/" in p:
        return "viewer.misc"
    if "/llcommon/" in p:
        return "core"
    if "/llmessage/" in p:
        return "network"
    return "unknown"


def main() -> None:
    out_dir = ROOT / "docs" / "architecture" / "generated"
    out_dir.mkdir(parents=True, exist_ok=True)

    rows = []

    for path in ROOT.rglob("*"):
        if not path.is_file():
            continue
        if should_skip(path):
            continue
        if path.suffix.lower() not in SOURCE_EXTS:
            continue

        rel = path.relative_to(ROOT).as_posix()

        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue

        lines = text.count("\n") + 1
        includes = INCLUDE_RE.findall(text)

        counts = {
            name: len(pattern.findall(text))
            for name, pattern in PATTERNS.items()
        }

        category = guess_category(rel, counts)

        rows.append({
            "path": rel,
            "category_guess": category,
            "lines": lines,
            "includes": len(includes),
            **counts,
        })

    rows.sort(key=lambda r: (r["gl_calls"], r["lines"]), reverse=True)

    csv_path = out_dir / "source_inventory.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()) if rows else ["path"])
        writer.writeheader()
        writer.writerows(rows)

    md_path = out_dir / "source_inventory_top.md"
    with md_path.open("w", encoding="utf-8") as f:
        f.write("# Source inventory — top OpenGL-related files\n\n")
        f.write("| file | category | lines | gl_calls | gGL | LLGL | LLPipeline | LLRenderTarget |\n")
        f.write("|---|---:|---:|---:|---:|---:|---:|---:|\n")

        for row in rows[:100]:
            f.write(
                f"| `{row['path']}` "
                f"| {row['category_guess']} "
                f"| {row['lines']} "
                f"| {row['gl_calls']} "
                f"| {row['gGL']} "
                f"| {row['LLGL']} "
                f"| {row['LLPipeline']} "
                f"| {row['LLRenderTarget']} |\n"
            )

    print(f"Wrote {csv_path}")
    print(f"Wrote {md_path}")
    print(f"Scanned {len(rows)} source files")


if __name__ == "__main__":
    main()
