# Guardrail Review After Header Boundary Cleanup

Date: 2026-05-22

## Scope

This note reviews old guardrails that still mention avoiding `pipeline.cpp`,
`llrender`, `llui`, or header-level OpenGL exposure.

## Historical Guardrails

Many earlier architecture notes say not to touch `pipeline.cpp`, draw pools,
UI rendering, shader managers, or low-level render files. Those notes should
remain unchanged because they describe the risk boundary for the specific task
at the time.

Examples:

- phase 1 and phase 2 inventory notes avoided source behavior changes;
- early phase 3 `LLRenderTarget` tasks avoided `pipeline.cpp` so FBO ownership
  could be contained locally first;
- later pipeline-specific notes explicitly scoped the `pipeline.cpp`
  containment work.

## Current Status

The old "do not touch `pipeline.cpp` yet" guardrail is no longer a blanket
phase 3 rule.

It has been superseded by narrower requirements:

- `pipeline.cpp` may be touched only for a documented task with a specific
  callsite family or header-boundary reason.
- `pipeline.cpp` must still not be used as a place for broad renderer
  redesign, backend abstraction, or unrelated cleanup.
- Changes should keep targeted object compiles as the default validation path.

The header-level OpenGL exposure cleanup is complete for normal runtime
headers:

- direct `llgl.h` includes are limited to precompiled/prefix headers;
- raw GL scalar type names in headers are confined to `llglheaders.h` and
  comments;
- `check_gl_header_boundaries.py` now guards that state.

## Remaining Active Guardrails

- Do not move source files.
- Do not start a direct Vulkan port.
- Do not add broad or speculative render abstractions.
- Do not add new runtime `gl*` calls outside `LLGLContainment`.
- Keep `llgl.h` and `llglheaders.h` as the explicit low-level OpenGL boundary.
- Validate FSR2 implementation changes on a build where `MARE_ENABLE_FSR2` is
  active.

## Small Next Steps

- Run both guardrails before committing render boundary work:
  - `python3 tools/architecture/check_gl_containment.py .`
  - `python3 tools/architecture/check_gl_header_boundaries.py .`
- Prefer documentation/task notes before changing behavior.
- Keep future `pipeline.cpp` changes narrowly scoped and compile
  `pipeline.cpp.o` directly.
