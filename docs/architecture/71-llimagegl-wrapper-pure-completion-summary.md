# LLImageGL Wrapper-Pure Completion Summary

Branch: `phase3`

Base branch: `phase2`

Source commit: `9a4fb5668e`

## Completed Scope

The wrapper-pure `LLImageGL` call families from
`docs/architecture/70-llimagegl-wrapper-pure-containment-task.md` now route
through `llglcontainment.*`.

Moved families:

- texture object name generation and deletion
- debug texture integer queries
- texture residency query
- readback error drains
- texture sub-image upload calls
- texture integer parameter calls for base/max level
- texture swizzle integer-vector parameter calls

`LLImageGL` still owns texture pooling, delayed deletion, row batching,
readback policy, debug logging, residency state, upload decisions, and memory
accounting.

## Inventory Result

Generated source inventory after the patch:

- `indra/llrender/llimagegl.cpp`: likely `gl*` call expressions dropped from
  34 to 15
- `indra/llrender/llimagegl.cpp`: raw `gl*` references dropped from 47 to 28
- `indra/llrender/llglcontainment.cpp`: likely `gl*` call expressions rose
  from 34 to 41

The remaining `LLImageGL` matches are the contract-first families plus
comment-only or disabled references.

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

The targeted build only recompiled `llimagegl.cpp` and
`llglcontainment.cpp`, then relinked `libllrender.a`.

## Deferred Work

These families are intentionally still direct in `LLImageGL` until each gets
a focused contract:

- compressed full texture upload
- automatic mipmap generation policy
- full `glTexImage2D(...)` texture allocation/copy
- scale-down FBO path
- scale-down PBO path

The Xcode arm64 Release integration build and runtime smoke test are deferred
for this packet because the changes are direct call-through wrapper moves and
do not change ordering or ownership.

## Next Small Tasks

Recommended next tasks:

1. Write a focused contract for compressed full upload and mipmap generation.
2. Decide whether full `glTexImage2D(...)` allocation belongs in the same
   contract or a separate one.
3. Leave the scale-down FBO/PBO path last because it mixes viewport, draw,
   readback, allocation, mipmap, and memory accounting behavior.
