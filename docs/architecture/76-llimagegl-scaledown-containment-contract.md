# LLImageGL ScaleDown Containment Contract

Branch: `phase3`

Base branch: `phase2`

## Scope

This document covers the remaining active direct OpenGL calls in
`LLImageGL::scaleDown(...)`.

No source behavior changes are made by this document.

Remaining active calls:

- `glViewport(...)`
- `glDrawArrays(...)`
- FBO-path `glTexImage2D(...)`
- FBO-path `glGenerateMipmap(...)`
- PBO-path `glTexImage2D(...)`
- PBO-path `glGenerateMipmap(...)`

## Current Behavior

`scaleDown(...)` reduces the current texture to a larger discard level.

Shared behavior:

- rejects unsupported targets and uninitialized internal formats
- clamps requested discard level to `mMaxDiscardLevel`
- exits if the requested discard level is not higher than
  `mCurrentDiscardLevel`
- computes the source mip offset from discard levels
- updates `mCurrentDiscardLevel` only after the selected path completes

FBO-style path when `gGLManager.mDownScaleMethod == 0`:

1. Sets viewport to the desired smaller texture size.
2. Binds this texture for the downscale shader path.
3. Draws a full-screen triangle.
4. Frees texture memory accounting for the old texture name.
5. Reallocates level 0 with `glTexImage2D(..., nullptr)`.
6. Copies the current framebuffer into level 0.
7. Allocates texture memory accounting for the new size.
8. Marks texture options dirty.
9. Regenerates mipmaps when `mHasMipMaps` is true.
10. Unbinds the texture after mipmap generation.

PBO-style path when `gGLManager.mDownScaleMethod != 0`:

1. Binds this texture.
2. Ensures the scratch PBO exists.
3. Binds the scratch PBO as `GL_PIXEL_PACK_BUFFER`.
4. Resizes the scratch PBO when the requested size is larger.
5. Reads the selected mip level into the pack PBO.
6. Frees texture memory accounting for the old texture name.
7. Unbinds the pack PBO.
8. Binds the scratch PBO as `GL_PIXEL_UNPACK_BUFFER`.
9. Reallocates level 0 with `glTexImage2D(..., nullptr)`, using the unpack PBO
   source.
10. Unbinds the unpack PBO.
11. Allocates texture memory accounting for the new size.
12. Regenerates mipmaps when `mHasMipMaps` is true.
13. Unbinds the texture.

## Ownership Notes

`LLImageGL::scaleDown(...)` owns:

- downscale method selection
- discard-level math
- texture bind failure handling
- viewport size choice for the FBO path
- draw submission for the FBO path
- framebuffer-to-texture copy ordering
- scratch PBO size and binding order
- texture memory accounting around reallocation
- mipmap regeneration policy
- final `mCurrentDiscardLevel` mutation

`LLGLContainment` may only own raw GL calls if each path is split into a
separate source packet and the owner ordering remains visible in
`LLImageGL::scaleDown(...)`.

## Risk

Risk level: high.

Reasons:

- the FBO path touches viewport state and draw submission
- the FBO path depends on the currently bound framebuffer copy source
- the PBO path depends on pack/unpack binding order
- both paths reallocate texture storage while texture memory accounting is
  interleaved
- mipmap regeneration must happen before `mCurrentDiscardLevel` changes
- failures are currently only handled on FBO texture bind failure

## Required Split

Do not make one combined source patch for all remaining calls.

If this area is touched, split it as:

1. FBO-path viewport and draw containment only.
2. FBO-path reallocation and mipmap containment only.
3. PBO-path reallocation and mipmap containment only.

Each packet must be independently buildable and reviewable.

## Verification Requirement

Minimum for each source packet:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated source inventory

Required before considering `scaleDown(...)` containment complete:

- local incremental Xcode arm64 Release build
- launch to login screen
- load a texture-heavy scene
- resize the viewer window
- check for texture corruption after scene load and resize

This path should not rely only on `llrender/fast`.

## Stop Point

Current recommendation:

- stop phase 3 source wrapping here unless a stronger validation checkpoint is
  acceptable
- keep `scaleDown(...)` documented as the next high-risk boundary
- review `phase3` before moving into this path
