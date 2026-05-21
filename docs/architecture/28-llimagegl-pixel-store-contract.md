# LLImageGL Pixel Store Contract

This document records the current pixel store behavior in `LLImageGL`. It is a
phase 2 contract document and does not change source behavior by itself.

Related files:

- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llimagegl.h`

## Inventory

Original local calls in `indra/llrender/llimagegl.cpp` before the phase 2
source cleanup:

- `glPixelStorei(...)`: 12

After the phase 2 source cleanup:

- direct `glPixelStorei(...)` calls remain only in local helper bodies
- upload paths call named local helpers instead of spelling the OpenGL state
  mutation at every callsite

Pixel store names used:

- `GL_UNPACK_SWAP_BYTES`
- `GL_UNPACK_ROW_LENGTH`

## Ownership

Primary owner:

- `LLImageGL`

Methods involved:

- `LLImageGL::setImage(...)`
- `LLImageGL::setSubImage(...)`

Helper involved:

- `sub_image_lines(...)`

Pixel store is global OpenGL unpack state. `LLImageGL` owns the temporary
changes made for texture upload and partial update paths.

## Swap Bytes Contract

State:

- `GL_UNPACK_SWAP_BYTES`

Local owner flag:

- `mFormatSwapBytes`

Current behavior:

- when `mFormatSwapBytes` is true, `setImage(...)` sets
  `GL_UNPACK_SWAP_BYTES` to 1 before upload
- after that upload path, `setImage(...)` resets `GL_UNPACK_SWAP_BYTES` to 0
- each set/reset is followed by `stop_glerror()`
- this pattern appears in mip data upload, automatic mip generation, manual mip
  generation, non-mip upload, and `setSubImage(...)`

Contract:

- enabling swap bytes must remain scoped to the upload path that needs it
- every enabled path must restore `GL_UNPACK_SWAP_BYTES` to 0 before returning
  through normal control flow
- `stop_glerror()` placement around set/reset calls is part of the current
  behavior

## Row Length Contract

State:

- `GL_UNPACK_ROW_LENGTH`

Current behavior:

- `setSubImage(...)` sets `GL_UNPACK_ROW_LENGTH` to `data_width` before partial
  upload
- after the upload, `setSubImage(...)` resets `GL_UNPACK_ROW_LENGTH` to 0
- both set and reset are followed by `stop_glerror()`
- `sub_image_lines(...)` relies on caller-provided source pointer increments and
  does not own `GL_UNPACK_ROW_LENGTH`

Contract:

- row length changes are owned by `setSubImage(...)`
- row length must be reset to 0 before returning through normal control flow
- row length must not leak into later full texture uploads

## Relationship To Upload Paths

Pixel store state is used around:

- `LLImageGL::setManualImage(...)`
- `glTexSubImage2D(...)`
- `sub_image_lines(...)`

It does not own:

- texture name generation
- texture deletion
- texture memory accounting
- mipmap policy
- compressed texture upload selection

## Risk

Risk level: high.

Reasons:

- OpenGL pixel store state is global.
- A missing reset can corrupt later texture uploads.
- The same state is touched in several branches of `setImage(...)`.
- Error-check placement is interleaved with pixel store changes.
- `setImage(...)` has early failure paths in manual mip generation allocation
  code, so source cleanup must preserve control flow carefully.

## Not A Generic Containment API

Do not move pixel store handling to `llglcontainment.*` yet.

Reason:

- the local owner is `LLImageGL`
- swap-byte behavior depends on `mFormatSwapBytes`
- row-length behavior depends on `setSubImage(...)` source dimensions
- a generic wrapper would not know the upload path ownership or reset
  requirements

## Candidate Source Cleanup

The source cleanup is naming-only and remains inside:

- `indra/llrender/llimagegl.cpp`

Local helper names:

- `set_texture_unpack_swap_bytes_enabled(...)`
- `set_texture_unpack_row_length(...)`

Constraints:

- do not change public headers
- do not touch `LLTexUnit`
- do not change upload selection logic
- do not move code to `llglcontainment.*`
- do not alter error-check placement
- do not change early return behavior in `setImage(...)`

## Source Cleanup Applied

Applied in phase 2:

- added local `static` helpers for `GL_UNPACK_SWAP_BYTES` and
  `GL_UNPACK_ROW_LENGTH`
- replaced the existing upload-path `glPixelStorei(...)` callsites with those
  helpers
- kept `stop_glerror()` placement unchanged
- kept public headers unchanged
- kept `LLTexUnit` unchanged
- did not move behavior into `llglcontainment.*`

This does not change ownership: `LLImageGL` still owns these temporary unpack
state changes.

## Verification

Minimum:

- build `llrender/fast`: passed
- run `git diff --check`: passed

If behavior changes or non-`llrender` files are touched:

- run the local incremental Xcode arm64 Release build
- launch the viewer
- verify login screen
- load a texture-heavy scene
- check for swapped colors, corrupted partial updates, missing alpha masks, and
  broken UI/media textures
