# Phase 3 Viewport Containment Summary

This document summarizes the completed phase 3 render target viewport
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

- `LLGLContainment::setViewport(...)`

Raw OpenGL call now issued by `llglcontainment.*`:

- `glViewport(...)`

## Ownership Preserved

`LLRenderTarget` still owns:

- `set_render_target_viewport(...)`
- `restore_default_framebuffer_viewport()`
- render target viewport dimensions
- default framebuffer viewport restore from `gGLViewport`
- `sCurResX` and `sCurResY` updates
- `bindTarget()` / `flush()` ordering
- nested render target restore behavior

No public `LLRenderTarget` API changed.

## Expected Behavior Change

Expected behavior change:

- none

The packet changes where the raw viewport OpenGL call is issued, not which
viewport is selected or when target/default viewport state is restored.

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

Runtime login, loaded-scene, and resize smoke still need to be run on this
exact viewport packet build.

## Review Result

No blocking issue found in this packet from source and build checks.

The change remains within the phase 3 boundary:

- no viewer-window changes
- no camera changes
- no probe changes
- no `pipeline.cpp` changes
- no draw pool changes
- no UI rendering changes
- no shader manager changes
- no texture upload changes

## Inventory Result

After this packet:

- `indra/llrender/llrendertarget.cpp` has no direct `gl*` calls
- selected raw OpenGL calls are contained in `indra/llrender/llglcontainment.*`
- `LLRenderTarget` remains the owner of render target state and ordering

## Remaining Risk

Risk level: medium until runtime smoke is repeated.

Reasons:

- viewport affects visible 2D and 3D framing
- default framebuffer restore depends on `gGLViewport`
- runtime verification has not yet been repeated after this exact source
  packet

## Stop Point

Before declaring the `LLRenderTarget` phase 3 containment packet complete, run
the Xcode-built app and verify:

- login page
- loaded scene
- window resize
