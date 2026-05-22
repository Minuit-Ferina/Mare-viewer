# GL Containment Guardrail

Branch: `phase3`

## Purpose

Phase 3 centralized runtime OpenGL calls in `LLGLContainment`. The guardrail
keeps that work from regressing by failing when a new runtime `gl*` call appears
outside the approved boundary.

Script:

```text
tools/architecture/check_gl_containment.py
```

## Current Result

Command:

```sh
python3 tools/architecture/check_gl_containment.py .
```

Result on 2026-05-22:

```text
OK: no runtime gl* calls outside indra/llrender/llglcontainment.cpp.
Allowed runtime gl* calls in containment: 160
Non-runtime GL ABI declarations and loader names remain covered by source_inventory.py raw-reference reporting.
```

## What It Checks

The script scans source files under the repository and reports runtime
expressions matching:

```text
glFoo(...)
```

Allowed runtime path:

```text
indra/llrender/llglcontainment.cpp
```

It ignores:

- block comments
- line comments
- preprocessor lines
- `extern gl*` declarations
- function-pointer typedef declarations
- known non-OpenGL false positives such as `glTF` and `glView`

## What It Does Not Check

It does not fail raw GL references that are not runtime calls. Those remain
visible in `docs/architecture/generated/source_inventory.csv`.

Known raw-reference owners:

- `indra/llrender/llglheaders.h`: OpenGL ABI declarations
- `indra/llrender/llgl.cpp`: OpenGL symbol loading and loader names

This split is intentional. The guardrail protects executable callsites, while
the inventory continues to measure raw OpenGL vocabulary.

## When To Run

Run after any patch that touches:

- `indra/llrender/`
- `indra/newview/` renderer code
- shader, texture, vertex buffer, render target, draw pool, or pipeline files
- platform rendering bridge code

Recommended command group:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

## Failure Policy

If the script reports a callsite:

1. Confirm it is a real runtime OpenGL call.
2. Preserve the caller's current behavior and ordering.
3. Add or reuse a focused `LLGLContainment` wrapper.
4. Re-run the guardrail.

Do not add a broad allowlist entry unless the callsite is a deliberate
non-runtime ABI/loader reference and the source-inventory raw-reference report
still covers it.
