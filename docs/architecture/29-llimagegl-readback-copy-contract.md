# LLImageGL Readback And Copy Contract

This document records the current `LLImageGL` texture readback and framebuffer
copy behavior. It is phase 2 documentation and does not change source behavior.

Related files:

- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llimagegl.h`
- `docs/architecture/26-llimagegl-texture-lifecycle-map.md`

## Scope

This contract covers these local paths:

- `LLImageGL::readBackRaw(...)`
- `LLImageGL::setSubImageFromFrameBuffer(...)`
- `LLImageGL::scaleDown(...)`

It does not cover:

- texture name generation and deletion
- normal full texture upload
- normal partial texture upload from CPU memory
- public `LLImageGL` API changes
- cross-file containment through `llglcontainment.*`

## Inventory

Observed OpenGL families in the readback and copy paths:

| family | observed local use |
|---|---|
| `glGetTexLevelParameteriv` | query mip width, compressed flag, and compressed byte size in `readBackRaw(...)` |
| `glGetCompressedTexImage` | compressed readback in `readBackRaw(...)` |
| `glGetTexImage` | uncompressed readback in `readBackRaw(...)` and PBO readback in `scaleDown(...)` |
| `glGetError` | drains and reports errors before and after `readBackRaw(...)` |
| `glCopyTexSubImage2D` | framebuffer-to-texture copy in `setSubImageFromFrameBuffer(...)` and `scaleDown(...)` |
| `glBindBuffer` | PBO pack/unpack routing in `scaleDown(...)` |
| `glBufferData` | scratch PBO allocation in `scaleDown(...)` |
| `glViewport` | FBO-style downscale viewport in `scaleDown(...)` |
| `glDrawArrays` | FBO-style downscale draw in `scaleDown(...)` |

## Readback Ownership

Primary owner:

- `LLImageGL::readBackRaw(...)`

Current behavior:

- rejects invalid discard ranges before binding
- unbinds and manually rebinds the texture before querying
- queries mip width with `glGetTexLevelParameteriv(...)`
- validates expected width, height, and component count before readback
- optionally checks whether the texture is compressed
- drains existing GL errors before reading texture data
- allocates the destination `LLImageRaw` data after GL size checks
- uses `glGetCompressedTexImage(...)` for compressed data
- uses `glGetTexImage(...)` for uncompressed data
- checks GL errors after readback and deletes destination data on error

Risk level: high.

Reasons:

- readback consumes global GL error state
- compressed and uncompressed paths allocate different destination sizes
- failure paths may delete the destination image data
- binding is done manually and relies on the current texture unit state
- the compression query currently uses mip level 0 by construction, because
  the second argument is the local `is_compressed` value initialized to 0

## Framebuffer Copy Ownership

Primary owners:

- `LLImageGL::setSubImageFromFrameBuffer(...)`
- `LLImageGL::scaleDown(...)`

Current behavior:

- `setSubImageFromFrameBuffer(...)` binds this texture and copies from the
  current framebuffer with `glCopyTexSubImage2D(...)`
- successful copy marks `mGLTextureCreated` true
- `scaleDown(...)` has two paths selected by `gGLManager.mDownScaleMethod`
- method 0 uses a viewport change, a full-screen triangle draw, texture
  reallocation, then `glCopyTexSubImage2D(...)`
- non-zero method uses `sScratchPBO` as a pack/unpack bridge, reads a mip level
  with `glGetTexImage(...)`, reallocates level 0, and uploads from the PBO

Risk level: high.

Reasons:

- copy behavior depends on the currently bound framebuffer
- `scaleDown(...)` mixes viewport, draw, texture allocation, copy, PBO, and
  mipmap generation in one method
- the PBO path changes both `GL_PIXEL_PACK_BUFFER` and `GL_PIXEL_UNPACK_BUFFER`
- texture memory accounting is interleaved with GL allocation/copy calls

## Ordering Contract

For `readBackRaw(...)`:

- validation must happen before destination allocation
- existing GL errors are drained before readback
- post-readback GL errors must delete destination data and return false
- compressed readback must use the compressed byte-size query before allocation

For `setSubImageFromFrameBuffer(...)`:

- texture bind success gates the copy
- `mGLTextureCreated` is set only after a copy call on the successful path
- existing `stop_glerror()` placement must remain after the copy

For `scaleDown(...)`:

- viewport change happens before the method 0 draw path
- old texture memory accounting is freed before level 0 reallocation
- new texture memory accounting is allocated after level 0 reallocation/copy
- PBO pack binding must be reset before PBO unpack binding
- PBO unpack binding must be reset after reallocation from the PBO
- mipmap regeneration remains conditional on `mHasMipMaps`

## Not A Generic Containment API

Do not move these calls to `llglcontainment.*` yet.

Reason:

- readback owns `LLImageRaw` allocation and failure behavior
- copy paths depend on current framebuffer and viewport state
- `scaleDown(...)` mixes several unrelated GL state families
- a generic wrapper would hide ordering dependencies that still need to be
  reviewed locally

## Candidate Source Cleanup

Small naming-only cleanup is applied for the readback/query/copy callsites and
stays local to:

- `indra/llrender/llimagegl.cpp`

Local helper groups:

- texture readback query/read helpers for `readBackRaw(...)`
- framebuffer copy helper for `glCopyTexSubImage2D(...)`

Constraints:

- do not change public headers
- do not change allocation or error handling behavior
- do not change `gGLManager.mDownScaleMethod` selection
- do not move behavior into `llglcontainment.*`
- do not change texture memory accounting order
- do not change viewport, framebuffer, or texture binding assumptions

Not yet applied:

- scratch PBO pack/unpack binding helpers for `scaleDown(...)`

Reason:

- PBO pack/unpack binding is a separate global buffer-state family and should
  stay in a separate patch.

## Source Cleanup Applied

Applied in phase 2:

- added local helpers for texture-level parameter queries
- added local helpers for compressed and uncompressed texture readback
- added a local helper for current-framebuffer-to-texture copy
- replaced the documented readback/copy callsites with those helpers
- kept public headers unchanged
- kept allocation, error handling, texture memory accounting, and method
  selection unchanged
- did not move behavior into `llglcontainment.*`

This does not change ownership: `LLImageGL` still owns these readback and copy
paths.

## Verification

Minimum:

- build `llrender/fast`: passed
- run `git diff --check`: passed

If `scaleDown(...)`, framebuffer copy, PBO state, or readback failure behavior
changes:

- run the local incremental Xcode arm64 Release build
- launch the viewer
- verify login screen
- load a texture-heavy scene
- check texture previews, snapshots, UI/media textures, and any texture
  downscale-triggering scene for corruption
