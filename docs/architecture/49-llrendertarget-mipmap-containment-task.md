# LLRenderTarget Mipmap Containment Task

This document defines the next phase 3 source task before editing
`llglcontainment.*` again.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Related contracts:

- `docs/architecture/20-llrendertarget-mipmap-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/48-phase3-rendertarget-remaining-review.md`

## Task

Move only the raw render target mipmap OpenGL call behind
`llglcontainment.*`, while keeping `LLRenderTarget::flush()` as the owner of
render target completion ordering.

This is not a generic texture upload abstraction.

## Candidate API

Add one narrow function:

- `LLGLContainment::generateTextureMipmap(...)`

Intended raw operation:

- `glGenerateMipmap(texture_target)`

Reason for this shape:

- phase 2 already concentrated the raw call in
  `generate_bound_render_target_mipmaps()`
- `LLRenderTarget` still decides when mipmaps are generated
- `LLRenderTarget` still controls the texture binding immediately before the
  helper call
- `LLRenderTarget` still passes the texture target

## Ownership Boundary

`llglcontainment.*` may own:

- the raw `glGenerateMipmap(...)` call

`LLRenderTarget` must still own:

- the `flush()` ordering
- the `mGenerateMipMaps == LLTexUnit::TMG_AUTO` condition
- binding attachment 0 on texture channel 0
- trilinear filtering setup
- `GL_TEXTURE_2D` target selection
- render target stack restore after mipmap generation

## Source Patch Shape

Only this local helper body should change in the source patch:

- `generate_bound_render_target_mipmaps()`

Expected delegation:

- local mipmap helper calls the new `LLGLContainment` function
- local helper continues passing `GL_TEXTURE_2D`
- `flush()` call order remains unchanged

No public `LLRenderTarget` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- `mGenerateMipMaps` policy
- texture channel selection
- attachment 0 binding
- filtering setup
- render target stack restore
- FBO binding or buffer routing

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL call is issued, not when mipmaps are
generated or which texture target receives them.

## Review Checklist

Reviewers should check:

- `LLGLContainment` gains only one mipmap raw-call helper
- `LLRenderTarget` local helper still expresses render target mipmap intent
- `flush()` still generates mipmaps before leaving the render target scope
- the `mGenerateMipMaps == LLTexUnit::TMG_AUTO` condition is unchanged
- attachment 0 binding and filter setup are unchanged
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

Runtime scene smoke is useful but not required for this packet unless a source
diff changes more than the one local helper body.

## Source Patch Applied

Applied in phase 3:

- added `LLGLContainment::generateTextureMipmap(...)`
- delegated `generate_bound_render_target_mipmaps()` to the new containment
  helper
- kept `GL_TEXTURE_2D` selection in `LLRenderTarget`
- kept `mGenerateMipMaps` policy in `LLRenderTarget::flush()`
- kept attachment 0 binding and filter setup in `LLRenderTarget::flush()`
- kept render target stack restore behavior unchanged
- did not touch `pipeline.cpp`, draw pools, UI rendering, shader managers, or
  texture upload code

Generated inventory was regenerated after the source patch so
`docs/architecture/generated/source_inventory.csv` and
`docs/architecture/generated/source_inventory_top.md` reflect the new call
location.

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
