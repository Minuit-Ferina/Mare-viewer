# LLImageGL ScaleDown Containment Summary

Branch: `phase3`

Base branch: `phase2`

Source commits:

- `2d377900a3` FBO viewport and draw calls
- `4f0d73d8d6` FBO storage and mipmap calls
- `f05d9e85fc` PBO storage and mipmap calls

## Completed Scope

The remaining active direct OpenGL calls in `LLImageGL::scaleDown(...)` now
route through `llglcontainment.*`.

Moved FBO-path calls:

- viewport setup
- full-screen triangle draw
- level 0 texture reallocation
- post-downscale mipmap generation

Moved PBO-path calls:

- level 0 texture reallocation from the unpack PBO
- post-downscale mipmap generation

`LLImageGL::scaleDown(...)` still owns downscale method selection, discard
math, texture bind failure handling, framebuffer copy ordering, scratch PBO
binding order, texture memory accounting, mipmap policy, texture unbinds, and
final `mCurrentDiscardLevel` mutation.

## Inventory Result

Generated source inventory after the full `scaleDown(...)` packet:

- `indra/llrender/llimagegl.cpp`: likely `gl*` call expressions dropped from
  9 to 3 during the `scaleDown(...)` work
- `indra/llrender/llimagegl.cpp`: raw `gl*` references dropped from 22 to 16
- the remaining `LLImageGL` matches are inactive/comment-only:
  - explanatory `glSetSubImage2D(...)` comment
  - disabled manual mip tail `glTexParameteri(...)` block

No new `llglcontainment.*` helpers were needed for `scaleDown(...)`; the
packet reused existing viewport, draw, texture allocation, and mipmap helpers.

## Verification

Completed for each source packet:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

Integration checkpoint:

- moved `/private/tmp/Mare-viewer-phase2-xcode-worktree` to commit
  `f05d9e85fc`
- ran local incremental Xcode arm64 Release build without `-quiet`
- result: `BUILD SUCCEEDED`
- verified built executable is arm64
- verified app bundle contains:
  - `libopenal.dylib`
  - `libalut.dylib`
  - `libllwebrtc.dylib`
  - `libndofdev.dylib`

Non-fatal Xcode warnings remained consistent with previous local builds:

- CoreSimulator services unavailable
- `DARWIN_USER_CACHE_DIR` lookup failure, with Xcode fallback cache behavior
- CMake run script phases run every build

## Runtime Smoke

Runtime smoke is still required before treating the `scaleDown(...)` packet as
fully complete.

Required runtime checks:

- launch the Xcode-built app
- reach login screen
- load a texture-heavy scene
- resize the viewer window
- check for texture corruption after scene load and resize

Suggested app path:

```text
/private/tmp/Mare-viewer-phase2-xcode-worktree/build-darwin-universal-kokua-mkrlv/newview/Release/Mare Viewer.app
```

## Remaining OpenGL In LLImageGL

There are no active direct OpenGL calls left in `indra/llrender/llimagegl.cpp`
based on the local source scan after this packet.

The generated inventory still reports three likely `gl*` call expressions
because it counts inactive/comment-only matches.
