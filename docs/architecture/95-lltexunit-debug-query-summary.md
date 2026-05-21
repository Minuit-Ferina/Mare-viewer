# LLTexUnit Debug Query Containment Summary

Branch: `phase3`

Base branch: `phase2`

Source commit: `13c89efd8f`

## Completed Scope

The active texture query in `LLTexUnit::debugTextureUnit()` now routes through
`llglcontainment.*`.

Moved call:

- active texture integer query

`LLTexUnit` still owns the debug-only early return, expected/actual comparison,
and warning behavior.

## Inventory Result

Generated source inventory after the packet:

- `indra/llrender/llrender.cpp`: active direct `gl*` calls dropped from 42 to 41
- no new raw OpenGL helper was needed because `LLGLContainment::getInteger(...)`
  already existed

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

No Xcode or runtime smoke was run because this is pure wrapper routing.

## Next Candidate

Recommended next source packet:

- `LLTexUnit` texture parameter writes

Reason:

- the policy stays in `LLTexUnit`
- existing integer texture parameter containment can cover most calls
- only the anisotropy float parameter path needs a new narrow helper
