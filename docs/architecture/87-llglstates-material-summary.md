# LLGLStates Material Containment Summary

Branch: `phase3`

Base branch: `phase2`

Source commit: `8fefa5a89f`

## Completed Scope

Active direct fixed-function material calls in `indra/llrender/llglstates.h`
now route through `llglcontainment.*`.

Moved calls:

- specular material vector set
- shininess material integer set
- destructor specular reset
- destructor shininess reset

`LLGLSSpecular` still owns shininess conversion, clamp range, lifetime state,
and constructor/destructor ordering.

## Inventory Result

Generated source inventory after the packet:

- `indra/llrender/llglstates.h`: active direct `gl*` calls dropped to 0
- `indra/llrender/llglcontainment.cpp`: likely `gl*` call expressions rose
  from 48 to 50

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

No Xcode or runtime smoke was run because this is pure wrapper routing.

## Next Candidate

Recommended next small owner:

- `indra/llrender/llpostprocess.cpp`

Reason:

- it is the next compact `llrender` source file with active direct OpenGL calls
- it is smaller than `llrender.cpp`, `llgl.cpp`, shader managers, or
  `pipeline.cpp`
- it should be split into tiny families rather than handled as one broad patch
