# Phase 3 Allocation Error Containment Summary

This document summarizes the completed phase 3 render target allocation error
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

Added narrow containment helper:

- `LLGLContainment::getError()`

Raw OpenGL call now issued by `llglcontainment.*`:

- `glGetError()`

## Ownership Preserved

`LLRenderTarget` still owns:

- `render_target_texture_allocation_failed()`
- the `!= GL_NO_ERROR` allocation failure policy
- `clear_glerror()` placement
- `stop_glerror()` placement
- color and depth allocation warning text
- allocation `false` return behavior
- texture binding and image allocation ordering

No public `LLRenderTarget` API changed.

## Expected Behavior Change

Expected behavior change:

- none

The packet changes where the raw OpenGL error value is read, not when it is
consumed or how allocation failure is decided.

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

Risk level: low to medium.

Reasons:

- `glGetError()` consumes OpenGL error state
- this packet preserved the existing read location, but runtime coverage did
  not include texture allocation failure paths

## Stop Point

Remaining direct OpenGL family in `LLRenderTarget`:

- viewport

Viewport containment remains deferred until a loaded-scene and resize smoke
test is available.
