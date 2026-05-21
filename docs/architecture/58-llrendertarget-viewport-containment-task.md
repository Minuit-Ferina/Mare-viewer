# LLRenderTarget Viewport Containment Task

This document defines the final phase 3 `LLRenderTarget` source task before
editing `llglcontainment.*` again.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Related contracts:

- `docs/architecture/15-llrendertarget-viewport-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/57-phase3-pre-viewport-runtime-smoke.md`

## Task

Move only the raw render target viewport OpenGL calls behind
`llglcontainment.*`, while keeping `LLRenderTarget` as the owner of viewport
scope and restore policy.

This is not a generic viewer-window or camera viewport abstraction.

## Candidate API

Add one narrow function:

- `LLGLContainment::setViewport(...)`

Intended raw operation:

- `glViewport(x, y, width, height)`

Reason for this shape:

- phase 2 already concentrated the raw calls in two local helpers
- `LLRenderTarget` still decides which viewport is applied
- `LLRenderTarget` still restores the default framebuffer viewport from
  `gGLViewport`
- `LLGLContainment` only issues the raw OpenGL call

## Ownership Boundary

`llglcontainment.*` may own:

- the raw viewport call

`LLRenderTarget` must still own:

- `set_render_target_viewport(...)`
- `restore_default_framebuffer_viewport()`
- render target viewport dimensions
- default framebuffer viewport restore from `gGLViewport`
- `sCurResX` and `sCurResY` updates
- `bindTarget()` / `flush()` ordering
- nested render target restore behavior

## Source Patch Shape

Only these local helper bodies should change in the source patch:

- `set_render_target_viewport(...)`
- `restore_default_framebuffer_viewport()`

Expected delegation:

- local render target viewport helper calls the new containment function
- local default restore helper calls the new containment function
- local helpers keep `sCurResX` and `sCurResY` updates

No public `LLRenderTarget` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- `gGLViewport` reads
- `sCurResX` updates
- `sCurResY` updates
- default framebuffer restore policy
- render target stack restore behavior
- FBO binding
- draw/read buffer routing
- window, camera, UI, or probe viewport ownership

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL viewport call is issued, not which
viewport is selected or when target/default viewport state is restored.

## Review Checklist

Reviewers should check:

- `LLGLContainment` gains only one viewport raw-call helper
- `set_render_target_viewport(...)` still sets `(0, 0, mResX, mResY)`
- `restore_default_framebuffer_viewport()` still uses `gGLViewport`
- `sCurResX` and `sCurResY` updates are unchanged
- `bindTarget()` / `flush()` ordering is unchanged
- no viewer-window, camera, probe, draw-pool, `pipeline.cpp`, UI, shader, or
  texture upload code is touched

## Verification Plan

Minimum source verification:

- build `llrender/fast`
- run `git diff --check`
- regenerate generated source inventory

Because this is the final and riskiest `LLRenderTarget` direct-call packet,
also run:

- local incremental Xcode arm64 Release build
- executable architecture check
- runtime dylib presence check

After the build, perform a runtime smoke test:

- login page
- loaded scene
- window resize

## Source Patch Applied

Applied in phase 3:

- added `LLGLContainment::setViewport(...)`
- delegated `set_render_target_viewport(...)` to the new containment helper
- delegated `restore_default_framebuffer_viewport()` to the new containment
  helper
- kept `gGLViewport` reads in `LLRenderTarget`
- kept `sCurResX` and `sCurResY` updates in `LLRenderTarget`
- kept `bindTarget()` / `flush()` ordering unchanged
- did not touch viewer-window, camera, probe, draw-pool, `pipeline.cpp`, UI,
  shader, or texture upload code

Generated inventory was regenerated after the source patch so
`docs/architecture/generated/source_inventory.csv` and
`docs/architecture/generated/source_inventory_top.md` reflect the new call
location.

After this source patch, `indra/llrender/llrendertarget.cpp` has no direct
`gl*` calls.

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
