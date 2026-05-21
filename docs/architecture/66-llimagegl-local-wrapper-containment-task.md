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

## Source Patch Applied

Applied in phase 3:

- added opaque `LLGLsync` alias
- added `LLGLContainment::setPixelStoreInteger(...)`
- added `LLGLContainment::getTextureLevelParameterInteger(...)`
- added `LLGLContainment::readCompressedTextureImage(...)`
- added `LLGLContainment::readTextureImage(...)`
- added `LLGLContainment::copyTextureSubImage2D(...)`
- added `LLGLContainment::createSyncObject(...)`
- added `LLGLContainment::flushCommands()`
- added `LLGLContainment::clientWaitSyncObject(...)`
- added `LLGLContainment::waitSyncObject(...)`
- added `LLGLContainment::deleteSyncObject(...)`
- reused existing `LLGLContainment` buffer object helpers for scratch PBO name,
  bind, and resize operations
- delegated only the matching local `LLImageGL` helper bodies
- kept pixel-store branch ownership, readback allocation, scratch PBO state,
  sync branch differences, queue ordering, and texture-name handoff in
  `LLImageGL`
- did not touch upload, sub-image upload, mipmap, swizzle, parameter, texture
  name lifetime, `LLTexUnit`, UI, shader, draw-pool, or `pipeline.cpp` code

Generated inventory was regenerated after the source patch so
`docs/architecture/generated/source_inventory.csv` and
`docs/architecture/generated/source_inventory_top.md` reflect the new call
locations.

After this source patch:

- `indra/llrender/llimagegl.cpp` has 34 likely direct `gl*` calls in the
  generated inventory
- `indra/llrender/llglcontainment.cpp` has 34 likely direct `gl*` calls in the
  generated inventory

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

- rebuilt `llrender/CMakeFiles/llrender.dir/llglcontainment.cpp.o`
- rebuilt `llrender/CMakeFiles/llrender.dir/llimagegl.cpp.o`
- relinked `libllrender.a`

## Integration Build Check

The phase 3 `LLImageGL` local wrapper containment packet was also verified
with the local Xcode arm64 Release build.

Result:

- `xcodebuild -quiet` exited with code 0
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` present in the app bundle

Note:

- this Xcode build was much longer and quieter than expected, likely because
  more than the touched `llrender` objects rebuilt
- avoid using `-quiet` for future long Xcode checks when progress visibility
  matters

Detailed command and output notes are recorded in:

- `docs/architecture/local-darwin-arm64-build.md`

Runtime smoke is deferred for now by project decision on this wrapper-only
packet family.
