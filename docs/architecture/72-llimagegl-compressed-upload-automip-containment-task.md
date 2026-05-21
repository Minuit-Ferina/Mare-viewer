# LLImageGL Compressed Upload And Auto-Mipmap Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers the smallest remaining `LLImageGL::setImage(...)`
contract-first family that can still be routed as direct call-through
containment:

- compressed full texture upload with `glCompressedTexImage2D(...)`
- legacy automatic mipmap enable with `GL_GENERATE_MIPMAP`
- core-profile automatic mipmap generation with `glGenerateMipmap(...)`

The owner remains `LLImageGL::setImage(...)`. Containment must own only the raw
OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- full `glTexImage2D(...)` allocation/copy in `setManualImage(...)`
- texture memory accounting
- alpha analysis or pick-mask updates
- manual CPU mip generation
- `scaleDown(...)`
- FBO or PBO downscale behavior
- public `LLImageGL` APIs

## Ownership Notes

`LLImageGL::setImage(...)` remains responsible for:

- deciding whether data is compressed
- walking supplied mip levels and moving `data_in`
- updating `mMipLevels`
- choosing legacy versus core-profile automatic mipmap behavior
- keeping `stop_glerror()` checks in their current locations
- keeping upload branch order unchanged

`LLGLContainment` may add only a narrow compressed texture upload helper. The
existing texture parameter and mipmap helpers should be reused for auto-mipmap
calls.

## Ordering Notes

The patch must preserve:

- `mMipLevels` updates before compressed mip uploads
- `data_in` pointer movement before the matching compressed mip upload
- `stop_glerror()` immediately after compressed uploads
- legacy `GL_GENERATE_MIPMAP` setup before level 0 upload
- core-profile mipmap generation after level 0 upload and pick-mask updates

## Verification Plan

- Run `git diff --check`.
- Run the targeted `llrender/fast` build.
- Regenerate `docs/architecture/generated/source_inventory.csv`.
- Confirm `LLImageGL` direct `gl*` call expressions drop only by the scoped
  upload and auto-mipmap calls.

The local Xcode arm64 Release build and runtime smoke test remain deferred
unless an integration checkpoint is explicitly requested. This packet changes
only direct OpenGL call routing, not upload branch selection or texture memory
accounting.
