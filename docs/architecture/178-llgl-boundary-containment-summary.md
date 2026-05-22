# LLGL Boundary Containment Summary

Branch: `phase3`
Source commit: `962b6b0d67`

## Scope Completed

Addressed the remaining low-level GL inventory entries:
- `indra/llrender/llgl.cpp`
- `indra/llrender/llglheaders.h`

## Changes

`llgl.cpp`:
- routed executable GL manager probing through `LLGLContainment`
- routed debug callback state queries through `LLGLContainment`
- routed GL info string reads through `LLGLContainment`
- routed GL error drains through `LLGLContainment`
- routed `LLGLState` enable/disable/check paths through `LLGLContainment`
- routed `LLGLDepthTest` state changes/checks through `LLGLContainment`
- routed `LLGLSyncFence` sync operations through `LLGLContainment`
- kept GL symbol loading and `GLH_EXT_GET_PROC_ADDRESS` assignments unchanged

`llglcontainment.*`:
- added narrow wrappers for buffer parameter queries, boolean queries, hints,
  indexed strings, client active texture, depth state, and sync wait status

`tools/architecture/source_inventory.py`:
- stopped counting comments, `extern gl*` prototypes, and function-pointer
  typedef declarations as runtime `gl_calls`

`llglheaders.h`:
- left source unchanged
- now reports zero runtime `gl_calls` because its visible `gl*` entries are
  declarations, not executable callsites

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- `llrender/fast` rebuilt `llglcontainment.cpp` and `llgl.cpp`, then linked
  `libllrender.a`.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/llrender/llgl.cpp`: active direct `gl*` calls reduced from 79 to 0.
- `indra/llrender/llglheaders.h`: runtime `gl_calls` reduced from 36 to 0.
- `indra/llrender/llglheaders.h`: raw refs remain at 762, as expected for GL
  ABI declarations.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 151 to 159 because it now owns these call-through wrappers.

Runtime smoke:
- deferred unless requested.
