# Phase 3 Clear/Scissor Containment Summary

This document summarizes the completed phase 3 render target clear/scissor
containment packet.

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

- `LLGLContainment::clearBuffers(...)`
- `LLGLContainment::setScissorBox(...)`

Raw OpenGL calls now issued by `llglcontainment.*`:

- `glClear(...)`
- `glScissor(...)`

## Ownership Preserved

`LLRenderTarget` still owns:

- `LLRenderTarget::clear()` ordering
- render target clear mask calculation
- `mUseDepth` depth inclusion policy
- caller `mask_in` intersection
- `mFBO` branch policy
- framebuffer status checking
- fallback `LLGLEnable scissor(GL_SCISSOR_TEST)` scope
- fallback dimensions from `mResX` and `mResY`
- `stop_glerror()` placement

No public `LLRenderTarget` API changed.

## Expected Behavior Change

Expected behavior change:

- none

The packet changes where the raw clear/scissor OpenGL calls are issued, not
what buffers are cleared or which scope owns fallback scissor state.

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

Runtime loaded-scene smoke was not rerun for this packet.

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

- clear/scissor changes affect visible render target contents
- fallback scissor depends on global scissor enable state
- runtime verification did not include a fresh loaded-scene pass after this
  exact packet

## Stop Point

Remaining direct OpenGL families in `LLRenderTarget`:

- viewport
- texture allocation error check

Before touching viewport, run a loaded-scene smoke test with the Xcode-built
app from this packet.
