# LLImageGL Remaining Call Classification

This document classifies the remaining direct OpenGL call expressions in
`indra/llrender/llimagegl.cpp` after the phase 3 local wrapper containment
packet.

Branch: `phase3`

Base branch: `phase2`

## Inventory Baseline

Generated inventory row after phase 3 wrapper containment:

- file: `indra/llrender/llimagegl.cpp`
- likely `gl*` calls: 34
- raw `gl*` references: 47

Local source scan:

- active direct OpenGL calls: 31
- inactive/comment-only matches: 3

The remaining calls are not all equally risky. Several can still be moved as
plain wrapper relocations, but they should be split by family.

## Wrapper-Pure Candidates

These can likely be moved with the same phase 3 pattern if each family gets a
small task note first.

| family | calls | risk | reason |
|---|---:|---|---|
| texture object names | 3 | low-medium | `LLImageGL` already owns name pooling and delayed deletion; `llglcontainment.*` can own only `glGenTextures(...)` / `glDeleteTextures(...)`. |
| debug texture queries | 4 | low | `checkTexSize(...)` is debug-only; raw `glGetIntegerv(...)` and `glGetTexLevelParameteriv(...)` can be wrapped without moving validation policy. |
| readback error drains | 3 | low-medium | `glGetError()` consumes error state, but `LLImageGL` can keep the before/after readback drain policy local. |
| texture residency query | 1 | low-medium | `LLImageGL` still owns `mIsResident`; containment can own only `glAreTexturesResident(...)`. |
| texture sub-image calls | 3 | medium | batching and row policy stay local; the raw `glTexSubImage2D(...)` calls can be wrapped if branch order remains unchanged. |
| texture parameter integer calls | 2 | medium | base/max level policy remains tied to newly generated texture names; raw `glTexParameteri(...)` can be wrapped. |
| texture swizzle parameter calls | 3 | medium | format conversion policy remains local; raw `glTexParameteriv(...)` can be wrapped after a focused task note. |

Recommended first wrapper-pure packet:

- texture object names

Reason:

- it mirrors the completed FBO and VBO name lifetime packets
- ownership is already documented in
  `docs/architecture/27-llimagegl-texture-name-lifetime-contract.md`
- it is narrower than upload, mipmap, swizzle, sub-image, or scale-down paths

## Contract-First Candidates

These may still become wrapper relocations, but they are mixed with enough
local policy that they should not be patched directly from this classification.

| family | calls | risk | reason |
|---|---:|---|---|
| compressed full upload | 2 | medium-high | calls are inside compressed upload and mip/discard branches with `mMipLevels`, data pointer movement, and `stop_glerror()` policy. |
| automatic mipmap generation | 2 | medium-high | legacy `GL_GENERATE_MIPMAP` and core `glGenerateMipmap(...)` are tied to `mAutoGenMips`, `LLRender::sGLCoreProfile`, and upload branch ordering. |
| full texture allocation/copy | 2 | high | `glTexImage2D(...)` sits between `free_cur_tex_image()` and `alloc_tex_image(...)`; memory accounting order is part of behavior. |
| scale-down FBO path | 4 | high | mixes viewport, draw, texture reallocation, framebuffer copy, mipmap generation, bind/unbind, and texture memory accounting. |
| scale-down PBO path | 2 | high | mixes PBO readback, texture reallocation, unpack binding, mipmap generation, and memory accounting. |

These should each get a separate contract before source edits.

## False Positives And Deferred Matches

These matches should not drive a source patch:

- comment-only `glSetSubImage2D(...)` reference in the `sub_image_lines(...)`
  comment
- disabled block-comment `glTexParameteri(...)` calls near the old manual mip
  tail

## Recommended Order

Recommended wrapper-pure order:

1. Texture object name generation/deletion.
2. Debug texture queries and texture residency query.
3. Readback error drains.
4. Texture sub-image calls.
5. Texture parameter integer calls.
6. Texture swizzle parameter calls.

Recommended contract-first order:

1. Compressed full upload and automatic mipmap generation.
2. Full texture allocation/copy.
3. Scale-down FBO and PBO paths.

## Stop Point

Do not start the contract-first families until the wrapper-pure list is either
completed or explicitly deferred.
