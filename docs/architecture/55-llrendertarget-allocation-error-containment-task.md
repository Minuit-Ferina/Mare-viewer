# LLRenderTarget Allocation Error Containment Task

This document defines the next phase 3 source task before editing
`llglcontainment.*` again.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Related contracts:

- `docs/architecture/23-llrendertarget-texture-allocation-error-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/54-phase3-clear-login-smoke.md`

## Task

Move only the raw OpenGL error read behind `llglcontainment.*`, while keeping
`LLRenderTarget` as the owner of texture allocation failure policy.

This is not a generic debug or error policy abstraction.

## Candidate API

Add one narrow function:

- `LLGLContainment::getError()`

Intended raw operation:

- `glGetError()`

Reason for this shape:

- phase 2 already concentrated the raw call in
  `render_target_texture_allocation_failed()`
- `LLRenderTarget` still decides that any non-`GL_NO_ERROR` value means
  allocation failure
- `LLRenderTarget` still owns warning text and false return behavior
- `LLGLContainment` only reads the raw OpenGL error value

## Ownership Boundary

`llglcontainment.*` may own:

- the raw `glGetError()` call

`LLRenderTarget` must still own:

- `render_target_texture_allocation_failed()`
- the `!= GL_NO_ERROR` policy
- `clear_glerror()` placement
- `stop_glerror()` placement
- allocation warning text
- allocation `false` return behavior
- texture binding and image allocation ordering

## Source Patch Shape

Only this local helper body should change in the source patch:

- `render_target_texture_allocation_failed()`

Expected delegation:

- local allocation helper calls the new `LLGLContainment` function
- local allocation helper keeps the boolean policy
- allocation call order remains unchanged

No public `LLRenderTarget` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- allocation failure boolean policy
- color versus depth allocation warning text
- allocation return behavior
- `clear_glerror()` calls
- `stop_glerror()` calls
- texture binding, filtering, or byte accounting

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL error value is read, not when it is
consumed or how allocation failure is decided.

## Review Checklist

Reviewers should check:

- `LLGLContainment` gains only one raw error read helper
- `render_target_texture_allocation_failed()` still returns
  non-`GL_NO_ERROR`
- `clear_glerror()` placement is unchanged
- warning text and false return behavior are unchanged
- no texture allocation ordering changed
- no draw-pool, `pipeline.cpp`, UI, shader, or texture upload code is touched

## Verification Plan

Minimum source verification:

- build `llrender/fast`
- run `git diff --check`
- regenerate generated source inventory

Because this is another behavior-bearing `llglcontainment.*` packet, also run:

- local incremental Xcode arm64 Release build
- executable architecture check
- runtime dylib presence check

Runtime loaded-scene smoke is not required for this packet. Viewport remains
deferred until a loaded-scene and resize smoke test is available.

## Source Patch Applied

Applied in phase 3:

- added `LLGLContainment::getError()`
- delegated `render_target_texture_allocation_failed()` to the new
  containment helper
- kept `!= GL_NO_ERROR` policy in `LLRenderTarget`
- kept `clear_glerror()` and `stop_glerror()` placement unchanged
- kept color/depth allocation warning text and false return behavior unchanged
- did not touch `pipeline.cpp`, draw pools, UI rendering, shader managers, or
  texture upload code

Generated inventory was regenerated after the source patch so
`docs/architecture/generated/source_inventory.csv` and
`docs/architecture/generated/source_inventory_top.md` reflect the new call
location.

## Source Build Check

Date: 2026-05-21 CEST

Targeted build:

```sh
CLANG_MODULE_CACHE_PATH=/private/tmp/Mare-viewer-phase2-llrender-make3/clang-module-cache \
/opt/homebrew/bin/cmake \
  --build /private/tmp/Mare-viewer-phase2-llrender-make3 \
  --target llrender/fast -- -j8
```

Result: passed.

Observed work:

- rebuilt `llrender/CMakeFiles/llrender.dir/llrendertarget.cpp.o`
- rebuilt `llrender/CMakeFiles/llrender.dir/llglcontainment.cpp.o`
- relinked `libllrender.a`
