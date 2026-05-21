# Phase 3 FBO Lifetime Containment Summary

This document summarizes the completed phase 3 FBO name lifetime containment
packet.

Branch: `phase3`

Base branch: `phase2`

## Source Scope

Touched source files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Touched generated files:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

## API Added

Added narrow containment helpers:

- `LLGLContainment::generateFramebuffers(...)`
- `LLGLContainment::deleteFramebuffers(...)`

Raw OpenGL calls now issued by `llglcontainment.*`:

- `glGenFramebuffers(...)`
- `glDeleteFramebuffers(...)`

## Ownership Preserved

`LLRenderTarget` still owns:

- the `mFBO` field
- `generate_framebuffer_name(...)`
- `delete_framebuffer_name(...)`
- allocation and release ordering
- attachment detach ordering
- texture deletion behavior
- `sCurFBO` safety reset before deleting a currently tracked FBO
- `mFBO = 0` after deletion
- `swapFBORefs()` behavior

No public `LLRenderTarget` API changed.

## Expected Behavior Change

Expected behavior change:

- none

The packet changes where the raw FBO name lifetime OpenGL calls are issued,
not when names are generated, deleted, or cleared from local owner state.

## Verification

Completed checks:

- `git diff --check`: passed
- targeted `llrender/fast`: passed
- generated source inventory regenerated
- local incremental Xcode arm64 Release build: passed
- executable verified as arm64
- runtime dylibs verified in the app bundle:
  - `libopenal.dylib`
  - `libalut.dylib`
  - `libllwebrtc.dylib`
  - `libndofdev.dylib`

Runtime scene smoke was not rerun for this packet because the previous phase 3
buffer routing packet already loaded a scene successfully and this packet only
moved FBO name generation/deletion calls behind `llglcontainment.*`.

## Review Result

No blocking issue found in this packet.

The change remains within the phase 3 boundary:

- no `pipeline.cpp` changes
- no draw pool changes
- no UI rendering changes
- no shader manager changes
- no texture upload changes

## Remaining Risk

Risk level: medium.

Reasons:

- FBO names remain OpenGL object lifetime state
- release ordering is still sensitive to `sCurFBO`
- runtime verification did not include a fresh loaded-scene pass after this
  exact packet

## Stop Point

Before moving another `LLRenderTarget` family into `llglcontainment.*`, review
the remaining direct calls and choose the next packet with the same documented
ownership, ordering, and verification constraints.
