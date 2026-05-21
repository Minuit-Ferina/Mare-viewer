# LLImageGL Compressed Upload And Auto-Mipmap Summary

Branch: `phase3`

Base branch: `phase2`

Source commit: `272d1aa6d4`

## Completed Scope

This packet routes the scoped `LLImageGL::setImage(...)` compressed upload and
automatic mipmap calls through `llglcontainment.*`.

Moved calls:

- compressed mip-level upload
- compressed level 0 upload
- legacy `GL_GENERATE_MIPMAP` parameter setup
- core-profile automatic mipmap generation after level 0 upload

The patch does not change upload branch selection, mip/discard state,
`stop_glerror()` placement, texture memory accounting, alpha analysis, or
pick-mask updates.

## Inventory Result

Generated source inventory after the patch:

- `indra/llrender/llimagegl.cpp`: likely `gl*` call expressions dropped from
  15 to 11
- `indra/llrender/llimagegl.cpp`: raw `gl*` references dropped from 28 to 24
- `indra/llrender/llglcontainment.cpp`: likely `gl*` call expressions rose
  from 41 to 42

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

The targeted build recompiled `llimagegl.cpp` and `llglcontainment.cpp`, then
relinked `libllrender.a`.

## Remaining Calls

Remaining active direct OpenGL calls in `LLImageGL` are concentrated in:

- `setManualImage(...)` full `glTexImage2D(...)` allocation/copy
- `scaleDown(...)` FBO-style viewport, draw, reallocation, copy, and mipmap
  regeneration
- `scaleDown(...)` PBO-style reallocation and mipmap regeneration

Remaining inactive matches:

- the `glSetSubImage2D(...)` explanatory comment
- disabled manual mip tail `glTexParameteri(...)` calls

## Next Stop Point

The next source packet should not combine `setManualImage(...)` and
`scaleDown(...)`.

Recommended next task:

1. Add a focused contract for `setManualImage(...)` `glTexImage2D(...)`
   allocation/copy containment.
2. Keep `free_cur_tex_image()` and `alloc_tex_image(...)` ownership local.
3. Leave `scaleDown(...)` last because it mixes viewport, draw, copy, PBO,
   allocation, mipmap regeneration, and discard-level state.
