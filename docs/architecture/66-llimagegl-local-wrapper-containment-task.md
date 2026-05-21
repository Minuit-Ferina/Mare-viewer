# LLImageGL Local Wrapper Containment Task

This document defines the next phase 3 wrapper relocation task after the
active `LLVertexBuffer` wrapper pass.

Branch: `phase3`

Base branch: `phase2`

## Task

Move the raw OpenGL calls already isolated behind local `LLImageGL` helper
functions into `llglcontainment.*`.

This task is limited to helpers introduced or mapped during phase 2:

- pixel store state
- texture readback queries and reads
- framebuffer-to-texture copy
- scratch PBO lifecycle, binding, and resize
- texture upload sync creation, waits, flush, and deletion

It does not touch full texture upload, sub-image upload, mipmap generation,
texture swizzle, texture parameters, texture name lifetime, or scale-down
allocation policy.

## Call Families

Move these local helper bodies only:

- `set_texture_unpack_swap_bytes_enabled(...)`
- `set_texture_unpack_row_length(...)`
- `query_texture_level_parameter(...)`
- `read_compressed_texture_level_image(...)`
- `read_texture_level_image(...)`
- `copy_current_framebuffer_to_texture_region(...)`
- `ensure_scratch_pbo_created(...)`
- `delete_scratch_pbo(...)`
- `bind_scratch_pbo_for_pixel_pack(...)`
- `unbind_pixel_pack_buffer()`
- `bind_scratch_pbo_for_pixel_unpack(...)`
- `unbind_pixel_unpack_buffer()`
- `resize_pixel_pack_buffer(...)`
- `create_texture_upload_sync()`
- `flush_texture_upload_commands()`
- `client_wait_for_texture_upload_sync(...)`
- `wait_for_texture_upload_sync(...)`
- `delete_texture_upload_sync(...)`

## Candidate API

Add narrow `LLGLContainment` helpers:

- `setPixelStoreInteger(...)`
- `getTextureLevelParameterInteger(...)`
- `readCompressedTextureImage(...)`
- `readTextureImage(...)`
- `copyTextureSubImage2D(...)`
- `createSyncObject(...)`
- `flushCommands()`
- `clientWaitSyncObject(...)`
- `waitSyncObject(...)`
- `deleteSyncObject(...)`

Reuse existing `LLGLContainment` buffer object helpers for scratch PBO name,
bind, and resize operations where possible.

Use an opaque `LLGLsync` alias rather than exposing `GLsync` from
`llglcontainment.h`.

## Ownership Boundary

`LLImageGL` must keep ownership of:

- `mFormatSwapBytes` enable/reset pairing
- `GL_UNPACK_ROW_LENGTH` reset ordering
- readback validation and allocation behavior
- compressed versus uncompressed readback branch policy
- framebuffer copy ownership and `mGLTextureCreated` update behavior
- `sScratchPBO`
- `sScratchPBOSize`
- scratch PBO creation/deletion guards
- scratch PBO pack/unpack binding order
- PBO resize policy
- NVIDIA versus non-NVIDIA sync branch differences
- main-thread callback order
- texture-name handoff and `ref()` / `unref()` lifetime

`llglcontainment.*` should own only the raw OpenGL calls.

## Explicitly Out Of Scope

Do not touch:

- `glTexImage2D(...)`
- `glCompressedTexImage2D(...)`
- `glTexSubImage2D(...)`
- `glTexParameteri(...)`
- `glTexParameteriv(...)`
- `glGenerateMipmap(...)` in `LLImageGL`
- texture name generation/deletion APIs
- texture memory accounting
- full upload branch selection
- manual mipmap allocation cleanup
- `scaleDown(...)` allocation policy
- `LLTexUnit`
- `indra/newview/`
- UI rendering
- shader managers
- draw pools
- `pipeline.cpp`

## Expected Behavior Change

Expected behavior change:

- none

The patch should change only where raw OpenGL calls are issued. It should not
change branch behavior, queue ordering, memory accounting, texture ownership,
or callback ordering.

## Verification Plan

Required:

- run `git diff --check`
- build `llrender/fast`
- regenerate generated source inventory

Because the source patch touches `llglcontainment.*` and `llimagegl.cpp`, also
run:

- local incremental Xcode arm64 Release build
- executable architecture check
- runtime dylib presence check

Runtime smoke:

- deferred for now by project decision on this wrapper-only packet family

## Stop Point

After this wrapper relocation, stop and summarize the `LLImageGL` phase 3
state before considering upload/mipmap/parameter containment.
