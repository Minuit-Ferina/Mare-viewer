# LLImageGL PBO And Sync Contract

This document records the current `LLImageGL` scratch PBO and GPU sync
behavior. It is phase 2 documentation and does not change source behavior.

Related files:

- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llimagegl.h`
- `docs/architecture/26-llimagegl-texture-lifecycle-map.md`
- `docs/architecture/29-llimagegl-readback-copy-contract.md`

## Scope

This contract covers:

- `LLImageGL::initClass(...)`
- `LLImageGL::cleanupClass()`
- `LLImageGL::allocateConversionBuffer()`
- `LLImageGL::scaleDown(...)`
- `LLImageGL::syncToMainThread(...)`
- `LLImageGL::syncTexName(...)`

It does not cover:

- normal texture upload policy
- normal texture readback allocation
- texture name pool allocation
- public `LLImageGL` API changes
- cross-file containment through `llglcontainment.*`

## Inventory

Observed OpenGL families in the scratch PBO and sync paths:

| family | observed local use |
|---|---|
| `glGenBuffers` | creates `sScratchPBO` in `initClass(...)` and lazily in `scaleDown(...)` |
| `glDeleteBuffers` | deletes `sScratchPBO` in `cleanupClass()` |
| `glBindBuffer` | binds and unbinds `GL_PIXEL_PACK_BUFFER` and `GL_PIXEL_UNPACK_BUFFER` in `scaleDown(...)` |
| `glBufferData` | resizes the scratch PBO in `scaleDown(...)` |
| `glFenceSync` | creates upload-completion sync objects in `syncToMainThread(...)` |
| `glClientWaitSync` | waits immediately on NVIDIA before main-thread notification |
| `glWaitSync` | waits from the posted main-thread callback on the non-NVIDIA path |
| `glDeleteSync` | deletes sync objects after wait |
| `glFlush` | flushes GPU work before sync waits or main-thread handoff |

## Scratch PBO Ownership

Primary owner:

- `LLImageGL` class static state

State:

- `LLImageGL::sScratchPBO`
- `LLImageGL::sScratchPBOSize`
- `LLImageGL::sManualScratch`

Current behavior:

- `initClass(...)` creates `sScratchPBO` if it is still 0
- `cleanupClass()` deletes `sScratchPBO`, then resets both `sScratchPBO` and
  `sScratchPBOSize`
- `allocateConversionBuffer()` allocates `sManualScratch` only for older GL
  versions and does not create GL buffer objects
- the PBO path in `scaleDown(...)` lazily creates `sScratchPBO` if needed
- the PBO path binds `sScratchPBO` as `GL_PIXEL_PACK_BUFFER` before
  `glGetTexImage(...)`
- if the requested size is larger than `sScratchPBOSize`, the PBO path resizes
  the buffer with `glBufferData(...)` and updates `sScratchPBOSize`
- after readback, the PBO path unbinds `GL_PIXEL_PACK_BUFFER`
- the same PBO is then bound as `GL_PIXEL_UNPACK_BUFFER` for level 0
  reallocation through `glTexImage2D(...)`
- after reallocation, the PBO path unbinds `GL_PIXEL_UNPACK_BUFFER`

Risk level: high.

Reasons:

- pixel pack/unpack buffer bindings are global OpenGL state
- one shared static PBO is reused across scale-down operations
- stale pack or unpack binding can affect later texture upload/readback paths
- buffer size tracking and GL buffer allocation must stay in sync
- this path is interleaved with texture memory accounting

## GPU Sync Ownership

Primary owner:

- `LLImageGL::syncToMainThread(...)`

Related owner:

- `LLImageGL::syncTexName(...)`

Current behavior:

- `syncToMainThread(...)` is called from non-main-thread texture creation when
  `defer_copy` is false
- NVIDIA path:
  - creates a sync object with `glFenceSync(...)`
  - flushes
  - waits immediately with `glClientWaitSync(...)`
  - deletes the sync object
- non-NVIDIA path:
  - flushes before creating the sync object
  - creates a sync object with `glFenceSync(...)`
  - flushes again
  - posts a main-thread callback that waits with `glWaitSync(...)`
  - deletes the sync object from the callback after the wait
- after the sync setup, `syncToMainThread(...)` posts a second main-thread
  callback that calls `syncTexName(new_tex_name)` and releases the image ref
- `syncTexName(...)` deletes the previous texture name if needed and installs
  the new texture name

Risk level: high.

Reasons:

- the NVIDIA and non-NVIDIA paths have deliberately different wait behavior
- the non-NVIDIA path depends on callback ordering on the main queue
- sync object deletion must happen after the matching wait
- texture-name handoff is tied to object ref/unref lifetime
- changing flush placement could reintroduce upload-completion races

## Ordering Contract

For scratch PBO usage in `scaleDown(...)`:

- create `sScratchPBO` before binding it
- bind `GL_PIXEL_PACK_BUFFER` before `glGetTexImage(...)`
- resize the PBO before readback if the requested size grew
- update `sScratchPBOSize` only after successful `glBufferData(...)` call
- unbind `GL_PIXEL_PACK_BUFFER` before binding `GL_PIXEL_UNPACK_BUFFER`
- unbind `GL_PIXEL_UNPACK_BUFFER` after reallocation from the PBO
- keep texture memory accounting around level 0 reallocation unchanged

For sync in `syncToMainThread(...)`:

- keep the NVIDIA immediate wait path separate from the non-NVIDIA posted wait
  path
- keep `glDeleteSync(...)` after the matching wait call
- keep the non-NVIDIA wait callback posted before the texture-name callback
- keep `ref()` before posting the texture-name callback and `unref()` inside
  that callback
- keep `syncTexName(...)` as the owner of old-name deletion during handoff

## Not A Generic Containment API

Do not move these calls to `llglcontainment.*` yet.

Reason:

- scratch PBO state is `LLImageGL` class state
- sync behavior depends on GL vendor behavior and main-thread queue ordering
- texture-name lifetime is part of the sync contract
- a generic wrapper would hide the ordering rules that still need local review

## Candidate Source Cleanup

Small naming-only cleanup is applied for the scratch PBO callsites and stays
local to:

- `indra/llrender/llimagegl.cpp`

Local helper groups:

- scratch PBO name generation/deletion helpers
- scratch PBO pack/unpack bind helpers
- scratch PBO resize helper

Constraints:

- do not change public headers
- do not change vendor-specific sync branching
- do not change main-thread callback order
- do not change texture-name handoff or ref/unref behavior
- do not change texture memory accounting order
- do not move behavior into `llglcontainment.*`

Not yet applied:

- sync create/wait/delete helpers local to `syncToMainThread(...)`

Reason:

- GPU sync uses vendor-specific branching and main-thread callback ordering, so
  it should stay in a separate patch.

## Source Cleanup Applied

Applied in phase 2:

- added local helpers for scratch PBO creation and deletion
- added local helpers for `GL_PIXEL_PACK_BUFFER` and
  `GL_PIXEL_UNPACK_BUFFER` bind/unbind state
- added a local helper for scratch PBO resize
- replaced the documented scratch PBO callsites with those helpers
- kept public headers unchanged
- kept texture memory accounting order unchanged
- did not move behavior into `llglcontainment.*`

This does not change ownership: `LLImageGL` still owns scratch PBO state.

## Verification

Minimum:

- build `llrender/fast`: passed
- run `git diff --check`: passed

If sync callback order, PBO binding, or texture-name handoff changes:

- run the local incremental Xcode arm64 Release build
- launch the viewer
- verify login screen
- load a texture-heavy scene
- watch for missing textures, stale textures, upload stalls, and crashes during
  texture loading
