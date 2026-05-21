# Phase 3 Mipmap Containment Summary

This document summarizes the completed phase 3 render target mipmap
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

- `LLGLContainment::generateTextureMipmap(...)`

Raw OpenGL call now issued by `llglcontainment.*`:

- `glGenerateMipmap(...)`

## Ownership Preserved

`LLRenderTarget` still owns:

- `generate_bound_render_target_mipmaps()`
- `LLRenderTarget::flush()` ordering
- the `mGenerateMipMaps == LLTexUnit::TMG_AUTO` condition
- texture channel 0 selection
- attachment 0 binding
- trilinear filtering setup
- `GL_TEXTURE_2D` target selection
- render target stack restore behavior

No public `LLRenderTarget` API changed.

## Expected Behavior Change

Expected behavior change:

- none

The packet changes where the raw mipmap OpenGL call is issued, not when
mipmaps are generated or which texture target receives them.

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
moved one render target mipmap raw call behind `llglcontainment.*`.

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

- mipmap generation depends on texture binding state
- runtime verification did not include a fresh loaded-scene pass after this
  exact packet
- only `LLRenderTarget` mipmap generation was contained; broader texture
  upload mipmap paths remain separate work

## Stop Point

Remaining direct OpenGL families in `LLRenderTarget`:

- viewport
- clear/scissor
- texture allocation error check

Before moving another family into `llglcontainment.*`, choose one packet with
the same documented ownership, ordering, and verification constraints.
