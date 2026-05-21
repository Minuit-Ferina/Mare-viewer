# LLRenderTarget FBO Lifetime Containment Task

This document defines the next phase 3 source task before editing
`llglcontainment.*` again.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Related contracts:

- `docs/architecture/19-llrendertarget-fbo-lifetime-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/45-phase3-rendertarget-review-summary.md`

## Task

Move only the raw FBO name lifetime OpenGL calls behind
`llglcontainment.*`, while keeping `LLRenderTarget` as the owner of FBO
ownership, release ordering, and local state.

This continues the phase 3 work inside the same owner and does not create a
generic object-lifetime abstraction.

## Candidate API

Add the narrowest possible functions:

- `LLGLContainment::generateFramebuffers(...)`
- `LLGLContainment::deleteFramebuffers(...)`

Intended raw operations:

- `glGenFramebuffers(...)`
- `glDeleteFramebuffers(...)`

Reason for this shape:

- phase 2 already concentrated the raw calls in
  `generate_framebuffer_name(...)` and `delete_framebuffer_name(...)`
- `LLRenderTarget` still owns the `mFBO` field and all state around it
- `LLGLContainment` only issues the raw OpenGL name lifetime calls

## Ownership Boundary

`llglcontainment.*` may own:

- raw framebuffer name generation
- raw framebuffer name deletion

`LLRenderTarget` must still own:

- the `mFBO` field
- release ordering
- attachment detach ordering
- depth and color texture ownership
- `sCurFBO` safety reset before deletion
- `mFBO = 0` after deletion
- `sBoundTarget` constraints
- `swapFBORefs()` ownership exchange behavior

## Source Patch Shape

Only these local helper bodies should change in the source patch:

- `generate_framebuffer_name(...)`
- `delete_framebuffer_name(...)`

Expected delegation:

- local FBO lifetime helpers call the new `LLGLContainment` functions
- local callers continue passing `&mFBO`
- release code still decides when deletion is legal and when local fields are
  reset

No public `LLRenderTarget` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- `release()` ordering
- `sCurFBO` comparison or reset
- binding framebuffer 0 before deleting the currently tracked FBO
- attachment detach behavior
- texture deletion behavior
- `mFBO = 0`
- any `sBoundTarget` assertion or stack behavior
- `swapFBORefs()` logic

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL calls are issued, not when an FBO name
is generated, deleted, or cleared from local owner state.

## Review Checklist

Reviewers should check:

- `LLGLContainment` gains only FBO name generation/deletion helpers
- `LLRenderTarget` local helper names still express lifetime intent
- `mFBO` remains owned and reset in `LLRenderTarget`
- release ordering is unchanged
- current-FBO safety reset remains before deletion
- attachment detach and texture deletion ordering are unchanged
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

Runtime scene testing is optional for this packet because the previous packet
already loaded a scene successfully, but it remains useful before leaving
`LLRenderTarget`.

## Source Patch Applied

Applied in phase 3:

- added `LLGLContainment::generateFramebuffers(...)`
- added `LLGLContainment::deleteFramebuffers(...)`
- delegated `generate_framebuffer_name(...)` to the new containment helper
- delegated `delete_framebuffer_name(...)` to the new containment helper
- kept `mFBO` ownership in `LLRenderTarget`
- kept `release()` ordering, `sCurFBO` safety reset, attachment detach,
  texture deletion, and `mFBO = 0` behavior in `LLRenderTarget`
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

The phase 3 FBO lifetime containment packet was also verified with the local
incremental Xcode arm64 Release build.

Result:

- `** BUILD SUCCEEDED **`
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` present in the app bundle

Detailed command and output notes are recorded in:

- `docs/architecture/local-darwin-arm64-build.md`
