# LLRender2D Line Width Containment Summary

Branch: `phase3`

Base branch: `phase2`

Source commit: `72e2be2c4d`

## Completed Scope

Active direct OpenGL calls in `indra/llrender/llrender2dutils.cpp` now route
through `llglcontainment.*`.

Moved calls:

- line-width range query
- temporary 3D line width set
- UI-scaled/clamped line width set

`LLRender2D` still owns flush ordering, cached line-width range state,
Darwin clamp behavior, UI scale factor use, and draw ordering.

## Inventory Result

Generated source inventory after the packet:

- `indra/llrender/llrender2dutils.cpp`: active direct `gl*` calls dropped to 0
- one raw generated reference remains from non-call text

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

No Xcode or runtime smoke was run because this is pure wrapper routing.

## Next Candidate

Recommended next small owner:

- `indra/llrender/llglstates.h`

Reason:

- four active direct fixed-function material calls remain there
- the file is small
- it is still lower risk than shader management, `llrender.cpp`, or
  `pipeline.cpp`
