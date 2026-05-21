# LLRenderTarget FBO Containment Task

This document defines the first phase 3 source task before editing
`llglcontainment.*`.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Related contracts:

- `docs/architecture/16-llrendertarget-fbo-contract.md`
- `docs/architecture/22-llrendertarget-framebuffer-status-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/36-phase3-plan.md`

## Task

Move only the raw framebuffer bind/status OpenGL calls behind
`llglcontainment.*`, while keeping `LLRenderTarget` as the owner of render
target state and policy.

This is not a generic renderer abstraction. It is a small containment step for
one OpenGL state family.

## Candidate API

Add the narrowest possible functions:

- `LLGLContainment::bindReadWriteFramebuffer(U32 framebuffer_name)`
- `LLGLContainment::getDrawFramebufferStatus()`

Intended raw operations:

- `glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_name)`
- `glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)`

Reason for this shape:

- all current `LLRenderTarget` binding helpers use `GL_FRAMEBUFFER`
- the status helper checks `GL_DRAW_FRAMEBUFFER`
- `LLRenderTarget` can keep its local intent helpers and delegate only the raw
  OpenGL calls
- `LLRenderTarget` can keep all tracker updates and debug policy local

## Ownership Boundary

`llglcontainment.*` may own:

- the raw `glBindFramebuffer(GL_FRAMEBUFFER, ...)` call
- the raw `glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)` call

`LLRenderTarget` must still own:

- `sCurFBO` updates
- `sBoundTarget` stack behavior
- default framebuffer restore policy
- attachment mutation restore policy
- viewport restore policy
- read/draw buffer routing policy
- `gDebugGL` conditional status checking
- warning text and `ll_fail(...)` behavior

## Source Patch Shape

Only these local helper bodies should change in the first source patch:

- `check_current_draw_framebuffer_status()`
- `bind_render_target_fbo(...)`
- `bind_attachment_fbo(...)`
- `restore_tracked_fbo_binding()`
- `bind_default_framebuffer_for_flush()`
- `forget_current_fbo_and_bind_default()`

Expected delegation:

- local FBO binding helpers call
  `LLGLContainment::bindReadWriteFramebuffer(...)`
- local status helper calls `LLGLContainment::getDrawFramebufferStatus()`

No public `LLRenderTarget` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- `LLRenderTarget::sCurFBO = ...`
- `LLRenderTarget::sBoundTarget = ...`
- `gDebugGL` checks
- `LL_WARNS()` messages
- `ll_fail(...)`
- viewport updates
- draw/read buffer routing
- texture attachment policy
- mipmap generation
- allocation error draining

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL calls are issued, not when they are
issued and not what local state is updated around them.

## Review Checklist

Reviewers should check:

- `LLRenderTarget` local helper names remain the intent layer
- `llglcontainment.*` exposes only the raw FBO bind/status operations
- `sCurFBO` assignment remains exactly where it was in local helpers
- attachment mutation helpers still do not update `sCurFBO`
- default framebuffer restore still sets `sCurFBO` to 0 in the same order
- status checking still runs only under `gDebugGL`
- status failure warning and `ll_fail(...)` stay in `LLRenderTarget`
- no draw-pool, `pipeline.cpp`, UI, shader, or texture upload code is touched

## Verification Plan

Minimum source verification:

- build `llrender/fast`
- run `git diff --check`

Because this is the first behavior-bearing `llglcontainment.*` packet, also
run:

- local incremental Xcode arm64 Release build
- executable architecture check
- runtime dylib presence check

Runtime smoke test is recommended if the viewer can be launched without
interrupting local work:

- login screen
- basic window resize
- one simple loaded scene

## Stop Point

After this source packet, stop and summarize the result before moving another
OpenGL family into `llglcontainment.*`.

## Source Patch Applied

Applied in phase 3:

- added `LLGLContainment::bindReadWriteFramebuffer(...)`
- added `LLGLContainment::getDrawFramebufferStatus()`
- delegated `LLRenderTarget` local FBO binding helper bodies to
  `LLGLContainment::bindReadWriteFramebuffer(...)`
- delegated `check_current_draw_framebuffer_status()` to
  `LLGLContainment::getDrawFramebufferStatus()`
- kept `LLRenderTarget` local helper names as the intent layer
- kept `sCurFBO` assignments in `LLRenderTarget`
- kept attachment mutation helpers from updating `sCurFBO`
- kept `gDebugGL`, warning text, and `ll_fail(...)` behavior in
  `LLRenderTarget`
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

The first phase 3 FBO containment packet was also verified with the local
incremental Xcode arm64 Release build.

Result:

- `** BUILD SUCCEEDED **`
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` present in the app bundle

Detailed command and output notes are recorded in:

- `docs/architecture/local-darwin-arm64-build.md`
