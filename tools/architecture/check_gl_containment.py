#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd().resolve()

SOURCE_EXTS = {
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".mm",
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

ALLOWED_RUNTIME_GL_CALL_PATHS = {
    "indra/llrender/llglcontainment.cpp",
}

KNOWN_GL_FALSE_POSITIVE_NAMES = {
    "glPointToScreen",
    "glReady",
    "glRectToScreen",
    "glTF",
    "glQuery",
    "glView",
}

GL_CALL_EXPR_RE = re.compile(r"\b(gl[A-Z][A-Za-z0-9_]*)\s*\(")
BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.DOTALL)


@dataclass(frozen=True)
class GLCall:
    path: str
    line_number: int
    name: str
    line: str


def should_skip(path: Path) -> bool:
    return any(part in SKIP_DIRS for part in path.parts)


def strip_block_comments(text: str) -> str:
    def replace(match: re.Match[str]) -> str:
        return "\n" * match.group(0).count("\n")

    return BLOCK_COMMENT_RE.sub(replace, text)


def strip_line_comment(line: str) -> str:
    return line.split("//", 1)[0]


def is_non_runtime_gl_call_line(line: str) -> bool:
    stripped = line.strip()
    if not stripped:
        return True
    if stripped.startswith("#"):
        return True
    if stripped.startswith("extern ") and GL_CALL_EXPR_RE.search(stripped):
        return True
    if stripped.startswith("typedef ") and "(*" in stripped:
        return True
    return False


def find_runtime_gl_calls(path: Path, rel: str) -> list[GLCall]:
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return []

    calls: list[GLCall] = []
    text = strip_block_comments(text)

    for line_number, raw_line in enumerate(text.splitlines(), start=1):
        line = strip_line_comment(raw_line)
        if is_non_runtime_gl_call_line(line):
            continue

        for name in GL_CALL_EXPR_RE.findall(line):
            if name in KNOWN_GL_FALSE_POSITIVE_NAMES:
                continue
            calls.append(GLCall(rel, line_number, name, raw_line.strip()))

    return calls


def iter_source_files(root: Path) -> list[Path]:
    paths: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if should_skip(path):
            continue
        if path.suffix.lower() not in SOURCE_EXTS:
            continue
        paths.append(path)
    return paths


def main() -> int:
    allowed_calls = 0
    violations: list[GLCall] = []

    for path in iter_source_files(ROOT):
        rel = path.relative_to(ROOT).as_posix()
        calls = find_runtime_gl_calls(path, rel)
        if rel in ALLOWED_RUNTIME_GL_CALL_PATHS:
            allowed_calls += len(calls)
            continue
        violations.extend(calls)

    if violations:
        print("OpenGL containment guardrail failed.", file=sys.stderr)
        print(
            "Runtime gl* calls are only allowed in "
            f"{', '.join(sorted(ALLOWED_RUNTIME_GL_CALL_PATHS))}.",
            file=sys.stderr,
        )
        for call in violations:
            print(
                f"{call.path}:{call.line_number}: {call.name}: {call.line}",
                file=sys.stderr,
            )
        return 1

    print(
        "OK: no runtime gl* calls outside "
        f"{', '.join(sorted(ALLOWED_RUNTIME_GL_CALL_PATHS))}."
    )
    print(f"Allowed runtime gl* calls in containment: {allowed_calls}")
    print(
        "Non-runtime GL ABI declarations and loader names remain covered by "
        "source_inventory.py raw-reference reporting."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
