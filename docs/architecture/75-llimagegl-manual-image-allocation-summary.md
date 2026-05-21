# LLImageGL Manual Image Allocation Summary

Branch: `phase3`

Base branch: `phase2`

Source commit: `2a913524a7`

## Completed Scope

This packet routes only the two `LLImageGL::setManualImage(...)`
`glTexImage2D(...)` calls through `llglcontainment.*`.

Moved calls:

- single-call texture allocation and copy
- texture allocation with `nullptr` before staggered `sub_image_lines(...)`
  copy

`LLImageGL::setManualImage(...)` still owns format conversion, compression
remapping, staggered upload selection, profiling zones, texture memory
accounting, and error-check placement.

## Inventory Result

Generated source inventory after the patch:

- `indra/llrender/llimagegl.cpp`: likely `gl*` call expressions dropped from
  11 to 9
- `indra/llrender/llimagegl.cpp`: raw `gl*` references dropped from 24 to 22
- `indra/llrender/llglcontainment.cpp`: likely `gl*` call expressions rose
  from 42 to 43

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

The targeted build recompiled `llimagegl.cpp` and `llglcontainment.cpp`, then
relinked `libllrender.a`.

## Remaining Active Calls

The remaining active direct OpenGL calls in `LLImageGL` are now concentrated in
`LLImageGL::scaleDown(...)`:

- FBO-style downscale viewport change
- FBO-style full-screen triangle draw
- FBO-style texture reallocation
- FBO-style post-downscale mipmap regeneration
- PBO-style texture reallocation
- PBO-style post-downscale mipmap regeneration

Inactive matches remain in:

- explanatory `glSetSubImage2D(...)` comment
- disabled manual mip tail `glTexParameteri(...)` block

## Stop Point

Do not route `scaleDown(...)` mechanically.

Reason:

- it mixes viewport state, draw submission, framebuffer copy, PBO pack/unpack,
  texture reallocation, mipmap regeneration, texture memory accounting, and
  discard-level mutation
- the FBO and PBO paths have different ordering assumptions
- this is the first remaining `LLImageGL` area where the call sequence itself
  is a multi-state render operation, not only a resource upload call

Recommended next task:

- write a focused `scaleDown(...)` containment contract before any source
  changes
- split FBO-style and PBO-style downscale into separate source packets if they
  are touched at all
- require an Xcode arm64 integration build before considering this complete
