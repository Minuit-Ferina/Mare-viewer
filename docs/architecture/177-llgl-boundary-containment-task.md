# LLGL Boundary Containment Task

Branch: `phase3`
Source owners:
- low-level GL manager
- GL state RAII helpers
- architecture inventory tool

## Scope

Address the remaining `llgl` inventory entries without changing GL behavior:
- `indra/llrender/llgl.cpp`
- `indra/llrender/llglheaders.h`

Included work:
- route executable `llgl.cpp` OpenGL callsites through `LLGLContainment`
- keep GL symbol loading and function-pointer declarations unchanged
- teach the inventory not to count `extern gl*` prototypes as runtime
  callsites

## Non-Scope

Do not change:
- GL extension lookup order
- GL capability decisions
- state-cache behavior
- debug/error policy
- sync fence timing
- platform GL symbol declarations

Do not rename OpenGL symbols in `llglheaders.h`.

## Ownership Notes

`llgl.cpp` keeps ownership of GL manager initialization, extension probing,
state RAII helpers, and error handling.

`llglheaders.h` keeps ownership of platform GL declarations and function-pointer
symbols. These names must continue to match the platform/OpenGL ABI.

`llglcontainment.*` owns the direct OpenGL call-through wrappers used by
executable code.

## Risk

Risk: medium.

Why:
- `llgl.cpp` is central to startup, capability detection, debug GL checks, and
  state RAII.
- The intended changes are wrapper-only, but this file is on a sensitive path.
- `llglheaders.h` should be fixed in the inventory accounting, not by source
  symbol renames.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested.
