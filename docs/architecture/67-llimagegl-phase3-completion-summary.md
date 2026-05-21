# LLImageGL Phase 3 Completion Summary

This document closes the phase 3 direct OpenGL containment pass for the local
`LLImageGL` wrapper helpers that were already isolated during phase 2.

Branch: `phase3`

Base branch: `phase2`

## Purpose

The purpose of this pass was to move raw OpenGL calls already isolated behind
local `LLImageGL` helper functions into `llglcontainment.*`, while keeping
texture ownership, upload policy, memory accounting, PBO state, and sync
ordering in `LLImageGL`.

This was a wrapper relocation pass, not a texture upload rewrite.

## Completed Packets

Completed behavior-bearing containment packet:

- pixel store:
  - `LLGLContainment::setPixelStoreInteger(...)`
- texture level queries:
  - `LLGLContainment::getTextureLevelParameterInteger(...)`
- texture readback:
  - `LLGLContainment::readCompressedTextureImage(...)`
  - `LLGLContainment::readTextureImage(...)`
- framebuffer copy:
  - `LLGLContainment::copyTextureSubImage2D(...)`
- scratch PBO support:
  - reused `LLGLContainment::generateBufferObjects(...)`
  - reused `LLGLContainment::deleteBufferObjects(...)`
  - reused `LLGLContainment::bindBufferObject(...)`
  - reused `LLGLContainment::allocateBufferObjectStorage(...)`
- texture upload sync:
  - `LLGLContainment::createSyncObject(...)`
  - `LLGLContainment::flushCommands()`
  - `LLGLContainment::clientWaitSyncObject(...)`
  - `LLGLContainment::waitSyncObject(...)`
  - `LLGLContainment::deleteSyncObject(...)`

## Source Scope

Touched source files:

- `indra/llrender/llgltypes.h`
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llimagegl.cpp`

Touched generated files:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

No public `LLImageGL` API changed.

## Ownership Preserved

`LLImageGL` still owns:

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
- texture-name handoff
- `ref()` / `unref()` lifetime around callbacks

`llglcontainment.*` owns only the raw OpenGL calls moved by this pass.

## Areas Not Touched

This pass did not edit:

- full texture upload through `glTexImage2D(...)`
- compressed full upload through `glCompressedTexImage2D(...)`
- partial upload through `glTexSubImage2D(...)`
- mipmap generation policy
- texture swizzle policy
- texture parameter policy
- texture name lifecycle APIs
- texture memory accounting
- `LLTexUnit`
- UI rendering
- shader managers
- draw pools
- `pipeline.cpp`

## Final Inventory Result

After this pass:

- `indra/llrender/llimagegl.cpp` has 34 likely direct `gl*` calls in the
  generated inventory
- `indra/llrender/llglcontainment.cpp` has 34 likely direct `gl*` calls in the
  generated inventory

Remaining `LLImageGL` direct calls are intentionally outside this wrapper
packet. They mostly belong to upload, sub-image upload, mipmap, texture
parameter, swizzle, debug texture-size, and scale-down allocation paths.

## Verification Completed

Completed checks:

- `git diff --check`
- targeted `llrender/fast`
- generated source inventory regeneration
- local Xcode arm64 Release build
- arm64 executable verification
- runtime dylib presence verification

Runtime smoke was intentionally deferred for this wrapper-only packet family.

## Expected Behavior Change

Expected behavior change:

- none

The pass changed where raw OpenGL calls are issued. It did not change branch
behavior, queue ordering, memory accounting, texture ownership, or callback
ordering.

## Remaining Risk

Risk level: medium-high.

Reasons:

- texture upload and scale-down paths still contain direct OpenGL calls
- scratch PBO and sync behavior remain global OpenGL state
- runtime smoke was deferred by project decision
- the Xcode integration build was longer than expected and used `-quiet`, so
  per-target rebuild detail was not visible

The risk is bounded because this pass only delegated already-isolated local
helper bodies to `llglcontainment.*`.

## Stop Point

Treat the local `LLImageGL` phase 3 wrapper relocation pass as complete.

Do not start upload, mipmap, swizzle, or texture parameter containment without
a separate task note and a stronger verification plan.
