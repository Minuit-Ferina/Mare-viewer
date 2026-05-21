# LLRenderTarget Clear Containment Task

This document defines the next phase 3 source task before editing
`llglcontainment.*` again.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Related contracts:

- `docs/architecture/21-llrendertarget-clear-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/51-phase3-rendertarget-next-choice.md`

## Task

Move only the raw render target clear/scissor OpenGL calls behind
`llglcontainment.*`, while keeping `LLRenderTarget::clear()` as the owner of
clear policy, mask construction, and fallback state scope.

This is not a generic frame clear abstraction.

## Candidate API

Add the narrowest possible functions:

- `LLGLContainment::clearBuffers(...)`
- `LLGLContainment::setScissorBox(...)`

Intended raw operations:

- `glClear(mask)`
- `glScissor(x, y, width, height)`

Reason for this shape:

- phase 2 already concentrated the raw calls in local helpers
- `LLRenderTarget` still decides which buffers to clear
- `LLRenderTarget` still owns the fallback scissor dimensions and enable scope
- `LLGLContainment` only issues the raw OpenGL calls

## Ownership Boundary

`llglcontainment.*` may own:

- the raw clear call
- the raw scissor rectangle call

`LLRenderTarget` must still own:

- `LLRenderTarget::clear()` ordering
- `mUseDepth` mask policy
- caller `mask_in` intersection
- `mFBO` branch policy
- framebuffer status checking
- `LLGLEnable scissor(GL_SCISSOR_TEST)` scope
- render target dimensions used for fallback scissor
- `stop_glerror()` placement

## Source Patch Shape

Only these local helper bodies should change in the source patch:

- `clear_render_target_buffers(...)`
- `set_render_target_scissor(...)`

Expected delegation:

- local clear helper calls the new `LLGLContainment` clear function
- local scissor helper calls the new `LLGLContainment` scissor function
- `LLRenderTarget::clear()` call order remains unchanged

No public `LLRenderTarget` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- mask calculation
- depth inclusion policy
- FBO versus fallback branch
- framebuffer status check
- scissor enable scope
- render target dimensions
- error-check placement
- clear color, depth value, stencil value, or frame orchestration policy

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL calls are issued, not what buffers are
cleared, when scissor is set, or which state scope owns fallback scissor.

## Review Checklist

Reviewers should check:

- `LLGLContainment` gains only clear/scissor raw-call helpers
- `LLRenderTarget` local helper names still express render target clear intent
- `LLRenderTarget::clear()` still computes the same mask
- `LLRenderTarget::clear()` still checks framebuffer status before FBO clear
- fallback path still uses `LLGLEnable scissor(GL_SCISSOR_TEST)`
- fallback scissor still uses `mResX` and `mResY`
- `stop_glerror()` placement is unchanged
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

Runtime loaded-scene smoke is recommended before moving to viewport because
clear behavior affects visible render target contents.

## Source Patch Applied

Applied in phase 3:

- added `LLGLContainment::clearBuffers(...)`
- added `LLGLContainment::setScissorBox(...)`
- delegated `clear_render_target_buffers(...)` to the new containment helper
- delegated `set_render_target_scissor(...)` to the new containment helper
- kept mask calculation in `LLRenderTarget::clear()`
- kept framebuffer status checking in `LLRenderTarget::clear()`
- kept fallback `LLGLEnable scissor(GL_SCISSOR_TEST)` scope in
  `LLRenderTarget::clear()`
- kept `stop_glerror()` placement unchanged
- did not touch `pipeline.cpp`, draw pools, UI rendering, shader managers, or
  texture upload code

Generated inventory was regenerated after the source patch so
`docs/architecture/generated/source_inventory.csv` and
`docs/architecture/generated/source_inventory_top.md` reflect the new call
locations.

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

## Integration Build Check

The phase 3 clear/scissor containment packet was also verified with the local
incremental Xcode arm64 Release build.

Result:

- `** BUILD SUCCEEDED **`
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` present in the app bundle

Detailed command and output notes are recorded in:

- `docs/architecture/local-darwin-arm64-build.md`
