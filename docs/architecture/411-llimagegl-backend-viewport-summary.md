# LLImageGL Backend Viewport Summary

## Scope

This packet routes the remaining direct viewport application in `LLImageGL`
through `LLRenderBackend`.

Runtime owner touched:

- `indra/llrender/llimagegl.cpp`

## What Moved

`LLImageGL::scaleDown()` now builds an `LLRenderViewport` for the temporary
downscale target and calls `getOpenGLRenderBackend().setViewport()`.

## What Did Not Move

This packet does not move:

- texture binding;
- texture allocation or upload;
- mipmap generation;
- draw call submission;
- FBO ownership;
- image thread checks.

Those still need dedicated backend vocabulary before they are good migration
candidates.

## Verification

- Built `llrender/CMakeFiles/llrender.dir/llimagegl.cpp.o`.
- Re-archived `llrender/libllrender.a`.
