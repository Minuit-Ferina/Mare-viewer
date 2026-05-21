# LLImageGL Upload Mipmap And Parameter Contract

This document records the remaining high-risk `LLImageGL` full upload, mipmap,
and texture-parameter behavior. It is phase 2 documentation and does not change
source behavior.

Related files:

- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llimagegl.h`
- `docs/architecture/26-llimagegl-texture-lifecycle-map.md`
- `docs/architecture/31-llimagegl-phase2-review-summary.md`

## Scope

This contract covers:

- `LLImageGL::setImage(...)`
- `LLImageGL::setManualImage(...)`
- `LLImageGL::createGLTexture(...)`
- `LLImageGL::scaleDown(...)`
- `LLImageGL::setAddressMode(...)`
- `LLImageGL::setFilteringOption(...)`

It does not cover:

- pixel store state already covered by
  `docs/architecture/28-llimagegl-pixel-store-contract.md`
- readback/copy/PBO/sync behavior already covered by
  `docs/architecture/29-llimagegl-readback-copy-contract.md` and
  `docs/architecture/30-llimagegl-pbo-sync-contract.md`
- public `LLImageGL` API changes
- cross-file containment through `llglcontainment.*`

## Inventory

Remaining OpenGL families in this area:

| family | observed local use |
|---|---|
| `glTexImage2D` | full allocation/copy in `setManualImage(...)`, level 0 reallocation in `scaleDown(...)` |
| `glCompressedTexImage2D` | compressed full upload in `setImage(...)` |
| `glTexSubImage2D` | partial CPU upload path and staggered line upload helper |
| `glTexParameteri` | legacy mipmap generation flag and base/max mip level settings |
| `glTexParameteriv` | core-profile swizzle conversion for deprecated pixel formats |
| `glGenerateMipmap` | core-profile auto mip generation and post-downscale mip regeneration |

## Full Upload Ownership

Primary owners:

- `LLImageGL::setImage(...)`
- `LLImageGL::setManualImage(...)`
- `LLImageGL::createGLTexture(...)`

Current behavior:

- `createGLTexture(...)` may reuse the existing texture name on the main thread
  when discard level and current texture state allow it
- otherwise it generates a new texture name, binds it, and initializes
  `GL_TEXTURE_BASE_LEVEL` and `GL_TEXTURE_MAX_LEVEL`
- `setImage(...)` owns the branch selection for null data, compressed data,
  supplied mip data, automatic mips, manual mip generation, and non-mip upload
- compressed full upload calls `glCompressedTexImage2D(...)` directly
- uncompressed full upload routes through `setManualImage(...)`
- `setManualImage(...)` owns deprecated-format conversion, optional texture
  compression remapping, texture memory accounting, and `glTexImage2D(...)`
- on selected platforms, `setManualImage(...)` can allocate with
  `glTexImage2D(..., nullptr)` and then upload through `sub_image_lines(...)`

Risk level: high.

Reasons:

- upload behavior combines format conversion, compression policy, memory
  accounting, and OpenGL allocation
- `setManualImage(...)` may use `sManualScratch` for fallback conversion
- changing upload order can break alpha masks, pick masks, media textures, or
  compressed textures
- texture memory accounting is interleaved with GL allocation

## Mipmap Ownership

Primary owners:

- `LLImageGL::setImage(...)`
- `LLImageGL::createGLTexture(...)`
- `LLImageGL::scaleDown(...)`

State:

- `mUseMipMaps`
- `mHasMipMaps`
- `mAutoGenMips`
- `mMipLevels`
- `mCurrentDiscardLevel`
- `mMaxDiscardLevel`

Current behavior:

- when `mUseMipMaps` is true, `setImage(...)` marks the texture as having
  mipmaps before binding and refreshes filtering state
- if caller data already has mipmaps, `setImage(...)` walks discard levels and
  uploads each level
- if mips are not supplied and `mAutoGenMips` is true, non-core profile can use
  `GL_GENERATE_MIPMAP`, while core profile calls `glGenerateMipmap(...)`
- if `mAutoGenMips` is false, `setImage(...)` creates mip levels manually on
  the CPU and uploads each level through `setManualImage(...)`
- `scaleDown(...)` regenerates mips after downscale when `mHasMipMaps` is true
- `createGLTexture(...)` sets `mAutoGenMips` true when `mUseMipMaps` is true

Risk level: high.

Reasons:

- legacy and core-profile mipmap paths differ
- manual mip generation has allocation failure paths
- discard level state affects texture dimensions and mip level numbering
- `scaleDown(...)` changes current discard level after reallocation

## Texture Parameter Ownership

Primary owners:

- `LLImageGL::createGLTexture(...)`
- `LLImageGL::setManualImage(...)`
- `LLImageGL::setAddressMode(...)`
- `LLImageGL::setFilteringOption(...)`

Current behavior:

- `createGLTexture(...)` initializes texture base/max level on newly generated
  texture names
- `setManualImage(...)` sets `GL_TEXTURE_SWIZZLE_RGBA` for core-profile
  conversion of deprecated `GL_ALPHA`, `GL_LUMINANCE`, and
  `GL_LUMINANCE_ALPHA` formats
- `setAddressMode(...)` marks texture options dirty and applies address mode
  immediately if this texture is currently bound
- `setFilteringOption(...)` marks texture options dirty and applies filtering
  immediately if this texture is currently bound
- `mTexOptionsDirty` records pending texture option application

Risk level: medium-high.

Reasons:

- parameter updates depend on which texture is currently bound
- swizzle conversion changes how deprecated formats are interpreted by shaders
- base/max mip level state affects sampling and discard behavior
- texture option dirtiness is shared with `LLTexUnit` behavior outside this file

## Ordering Contract

For full upload:

- texture name creation and binding must precede parameter setup
- `setImage(...)` branch selection must remain before upload calls
- `free_cur_tex_image()` must happen before `glTexImage2D(...)` allocation in
  `setManualImage(...)`
- `alloc_tex_image(...)` must happen after allocation/copy in
  `setManualImage(...)`
- alpha analysis and pick-mask updates must remain in their current upload
  branches

For mipmaps:

- mipmap state must be prepared before binding/upload when `mUseMipMaps` is
  true
- core-profile `glGenerateMipmap(...)` must remain after level 0 upload
- manual mip generation must preserve allocation failure cleanup and early
  return behavior
- `scaleDown(...)` must regenerate mips before updating current discard level

For texture parameters:

- base/max level setup must remain tied to newly generated texture names
- swizzle conversion must remain inside `setManualImage(...)`
- address/filter updates must continue to apply immediately only when the
  texture is currently bound

## Not A Generic Containment API

Do not move this behavior to `llglcontainment.*` yet.

Reason:

- upload owns `LLImageGL` format and memory accounting state
- mipmap policy depends on `LLImageGL` discard levels and profile-specific
  behavior
- texture parameters are coupled to `LLTexUnit` binding state
- a generic wrapper would hide ownership and ordering rules that still need
  local review

## Candidate Source Cleanup

Small naming-only cleanup may be reasonable later if it stays local to:

- `indra/llrender/llimagegl.cpp`

Possible helper groups:

- compressed texture upload helper
- full texture allocation/copy helper
- texture swizzle parameter helper
- texture mip level parameter helper
- mipmap generation helper

Constraints:

- do not change public headers
- do not change upload branch selection
- do not change memory accounting order
- do not change manual mip allocation cleanup
- do not change `LLTexUnit` behavior
- do not move behavior into `llglcontainment.*`

## Verification For A Future Source Patch

Minimum:

- build `llrender/fast`
- run `git diff --check`

If upload branch selection, mipmap generation, swizzle state, or texture
memory accounting changes:

- run the local incremental Xcode arm64 Release build
- launch the viewer
- verify login screen
- load a texture-heavy scene
- check media textures, UI textures, alpha textures, compressed textures, and
  downscaled textures for corruption or sampling changes
