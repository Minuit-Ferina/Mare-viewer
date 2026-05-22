# Remaining GL Boundaries Containment Summary

Branch: `phase3`
Source commit: `8541ef10ac`

## Scope Completed

Closed the remaining wrapper-only non-`pipeline.cpp` OpenGL inventory item:
- `indra/newview/marefsr2upscaler.cpp`

Classified as explicit low-level OpenGL boundary files:
- `indra/llrender/llgl.cpp`
- `indra/llrender/llglheaders.h`

## Changes

`marefsr2upscaler.cpp`:
- routed compute shader creation, source upload, compile, link, and deletion
  through `LLGLContainment`
- routed FSR2 direct-state texture creation, storage, and parameters through
  `LLGLContainment`
- routed image binding, texture-unit binding, compute dispatch, memory barriers,
  uniform updates, image copy, and program unbind through `LLGLContainment`
- kept FSR2 pass order, texture formats, shader paths, and dispatch group
  sizing unchanged

`llglcontainment.*`:
- added non-Darwin FSR2 compute and image helper wrappers
- kept those helpers out of the Darwin build path because local Darwin builds
  define `MARE_ENABLE_FSR2=0`

`llgl.cpp` and `llglheaders.h`:
- left unchanged as the GL loader, capability, and declaration boundary
- intentionally not routed through `llglcontainment.*`

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- `llrender/fast` rebuilt `llglcontainment.cpp` and linked `libllrender.a`.
- The local Darwin viewer target has `MARE_ENABLE_FSR2=0`, so
  `marefsr2upscaler.cpp` is not part of the local build graph.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/marefsr2upscaler.cpp`: active direct `gl*` calls reduced from
  81 to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 143 to 151 because it now owns the FSR2 call-through wrappers.
- `indra/llrender/llgl.cpp`: remains at 79 active direct `gl*` calls as an
  explicit loader/state boundary.
- `indra/llrender/llglheaders.h`: remains at 36 active direct `gl*` declarations
  as an explicit platform declaration boundary.

Runtime smoke:
- deferred unless requested.
