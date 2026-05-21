# LLImageGL Texture Name Lifetime Contract

This document records the current OpenGL texture name lifetime behavior in
`LLImageGL`. It is documentation only and does not change source behavior.

Related files:

- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llimagegl.h`

## Inventory

Current local calls in `indra/llrender/llimagegl.cpp`:

- `glGenTextures(...)`: 2
- `glDeleteTextures(...)`: 1

Existing public wrappers:

- `LLImageGL::generateTextures(...)`
- `LLImageGL::deleteTextures(...)`

The header already documents these as replacements for direct
`glGenTextures()` and `glDeleteTextures()` usage.

## Ownership

Primary owner:

- `LLImageGL`

Primary state:

- `mTexName`
- `mGLTextureCreated`
- `mCurrentDiscardLevel`
- `mTextureMemory`
- `sFrameCount`
- `sFreeList`

`mTexName` is the local GL texture name currently associated with an
`LLImageGL` instance. Most callers should treat it as owned by `LLImageGL`.

Exception:

- the constructor wrapping an existing `LLGLuint` texture name is explicitly
  for textures created elsewhere and should be used with caution
- `createGLTexture(..., usename, ...)` can bind a provided name on the main
  thread

## Generation Contract

Owner:

- `LLImageGL::generateTextures(...)`

Current behavior:

- maintains a thread-local pool of 1024 texture names
- refills the pool with `glGenTextures(pool_size, name_pool)` when empty
- satisfies small requests by copying names from the pool
- falls back to `glGenTextures(numTextures, textures)` when the request is
  larger than the remaining pool

Contract:

- callers should use `LLImageGL::generateTextures(...)`, not raw
  `glGenTextures(...)`
- generated names are not fully initialized textures until later upload or
  binding code initializes image storage and options
- generation does not update memory accounting by itself

## Deletion Contract

Owner:

- `LLImageGL::deleteTextures(...)`
- `LLImageGL::updateClass()`

Current behavior:

- `deleteTextures(...)` does not call `glDeleteTextures(...)` immediately
- names are queued into `sFreeList` using the current frame index
- `updateClass()` advances `sFrameCount`
- after `DELETE_DELAY` frames, `updateClass()` frees memory accounting for the
  queued names and calls `glDeleteTextures(...)`

Contract:

- deletion is delayed to avoid GPU synchronization issues
- callers should queue names through `LLImageGL::deleteTextures(...)`
- direct `glDeleteTextures(...)` is currently centralized in `updateClass()`
- freeing memory accounting and deleting GL names must stay paired

## Instance Replacement Contract

Owners:

- `LLImageGL::createGLTexture()`
- `LLImageGL::createGLTexture(..., const U8*, ...)`
- `LLImageGL::syncTexName(...)`
- `LLImageGL::destroyGLTexture()`

Current behavior:

- `createGLTexture()` deletes an existing `mTexName` before generating a new
  empty name
- `createGLTexture(..., const U8*, ...)` may reuse `mTexName` for same-size
  main-thread uploads, or create `new_texname`
- when replacing on the main thread, the old name is queued for delayed
  deletion before `mTexName` is updated
- when replacing from a loading thread, `syncToMainThread(...)` posts a main
  thread callback that eventually calls `syncTexName(...)`
- `syncTexName(...)` queues the old `mTexName` for delayed deletion before
  storing the new name
- `destroyGLTexture()` queues `mTexName` for delayed deletion, clears
  `mTextureMemory`, invalidates `mCurrentDiscardLevel`, sets `mTexName` to 0,
  and marks `mGLTextureCreated` false

Contract:

- replacing `mTexName` must not leak the old texture name
- `mTexName` should only be updated after the replacement path has handled old
  name deletion or deferral
- `destroyGLTexture()` invalidates the instance texture state, not just the GL
  object name

## Thread Handoff Contract

Owner:

- `LLImageGL::syncToMainThread(...)`

Current behavior:

- non-main-thread upload completion uses GPU sync before `mTexName` is swapped
- NVIDIA path waits on the sync immediately with `glClientWaitSync(...)`
- the other path posts a main-thread wait using `glWaitSync(...)`
- both paths delete the sync object with `glDeleteSync(...)`
- the texture name swap itself is posted to the main queue through
  `syncTexName(...)`

Contract:

- texture name replacement from non-main threads must remain synchronized with
  upload completion
- the final `mTexName` mutation must happen through the existing main-thread
  callback path
- sync handling is part of texture lifetime, not a generic render fence API yet

## Risk

Risk level: high.

Reasons:

- texture names cross main-thread and loading-thread paths
- deletion is intentionally delayed
- texture names interact with memory accounting and cache lifetime
- sync behavior differs by driver/vendor path
- `mTexName` can be wrapped from externally created GL state

## Not A Generic Containment API

Do not move texture name lifetime into `llglcontainment.*` yet.

Reason:

- `LLImageGL` already has public texture name lifecycle APIs
- deletion delay and memory accounting are local `LLImageGL` policy
- sync handoff is tied to `LLImageGL` work queues and `mTexName`
- a generic wrapper would hide ownership instead of clarifying it

## Source Cleanup Recommendation

No source cleanup is recommended for this exact family yet.

Reason:

- `glGenTextures(...)` and `glDeleteTextures(...)` are already centralized
  behind `LLImageGL::generateTextures(...)`, `LLImageGL::deleteTextures(...)`,
  and `LLImageGL::updateClass()`
- changing this path before runtime tests would carry unnecessary risk

The next safer phase 2 work is documenting pixel store ownership:

- `GL_UNPACK_SWAP_BYTES`
- `GL_UNPACK_ROW_LENGTH`
- reset requirements after upload and subimage paths

## Verification For Future Source Changes

Minimum for a future source cleanup:

- build `llrender/fast`
- run `git diff --check`

If source behavior changes:

- run the local incremental Xcode arm64 Release build
- launch the viewer
- verify login screen
- load a texture-heavy scene
- check for texture corruption, missing textures, and delayed deletion crashes
