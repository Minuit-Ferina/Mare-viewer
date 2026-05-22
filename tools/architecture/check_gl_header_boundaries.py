#!/usr/bin/env python3

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd().resolve()

HEADER_EXTS = {
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
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

ALLOWED_LLGL_INCLUDE_HEADERS = {
    "indra/newview/llviewerprecompiledheaders.h",
    "indra/newview/macview_Prefix.h",
}

ALLOWED_RAW_GL_TYPE_HEADERS = {
    "indra/llrender/llglheaders.h",
}

LLGL_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"llgl\.h"\s*(?://.*)?$')
RAW_GL_TYPE_RE = re.compile(
    r"\b(GLenum|GLuint|GLint|GLfloat|GLboolean|GLsizei|GLchar)\b"
)
BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.DOTALL)


@dataclass(frozen=True)
class Violation:
    path: str
    line_number: int
    kind: str
    line: str


def should_skip(path: Path) -> bool:
    return any(part in SKIP_DIRS for part in path.parts)


def strip_block_comments(text: str) -> str:
    def replace(match: re.Match[str]) -> str:
        return "\n" * match.group(0).count("\n")

    return BLOCK_COMMENT_RE.sub(replace, text)


def strip_line_comment(line: str) -> str:
    return line.split("//", 1)[0]


def iter_header_files(root: Path) -> list[Path]:
    paths: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if should_skip(path):
            continue
        if path.suffix.lower() not in HEADER_EXTS:
            continue
        paths.append(path)
    return paths


def find_violations(path: Path, rel: str) -> list[Violation]:
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return []

    violations: list[Violation] = []
    stripped_text = strip_block_comments(text)

    for line_number, raw_line in enumerate(stripped_text.splitlines(), start=1):
        if rel not in ALLOWED_LLGL_INCLUDE_HEADERS and LLGL_INCLUDE_RE.match(raw_line):
            violations.append(
                Violation(rel, line_number, "direct llgl.h include", raw_line.strip())
            )

        if rel not in ALLOWED_RAW_GL_TYPE_HEADERS:
            code_line = strip_line_comment(raw_line)
            for match in RAW_GL_TYPE_RE.finditer(code_line):
                violations.append(
                    Violation(
                        rel,
                        line_number,
                        f"raw GL scalar type {match.group(1)}",
                        raw_line.strip(),
                    )
                )

    return violations


def main() -> int:
    violations: list[Violation] = []

    for path in iter_header_files(ROOT):
        rel = path.relative_to(ROOT).as_posix()
        violations.extend(find_violations(path, rel))

    if violations:
        print("OpenGL header boundary guardrail failed.", file=sys.stderr)
        print(
            "Headers should prefer llgltypes.h aliases and keep llgl.h local "
            "to implementation files unless explicitly allowed.",
            file=sys.stderr,
        )
        print(
            "Allowed direct llgl.h include headers: "
            f"{', '.join(sorted(ALLOWED_LLGL_INCLUDE_HEADERS))}",
            file=sys.stderr,
        )
        print(
            "Allowed raw GL type headers: "
            f"{', '.join(sorted(ALLOWED_RAW_GL_TYPE_HEADERS))}",
            file=sys.stderr,
        )
        for violation in violations:
            print(
                f"{violation.path}:{violation.line_number}: "
                f"{violation.kind}: {violation.line}",
                file=sys.stderr,
            )
        return 1

    print("OK: runtime headers do not include llgl.h directly outside PCH/prefix.")
    print("OK: raw GL scalar type names are confined to llglheaders.h.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
