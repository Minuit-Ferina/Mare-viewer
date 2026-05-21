# LLPostProcess Containment Summary

Branch: `phase3`

Base branch: `phase2`

Source commits:

- `6c38b3bc8d`
- `59060ce092`
- `c576ba5104`

## Completed Scope

Active direct OpenGL calls in `indra/llrender/llpostprocess.cpp` now route
through `llglcontainment.*`.

Moved families:

- attribute stack push/pop and clear calls
- framebuffer-to-texture copy
- texture rectangle allocation
- uniform lookup
- OpenGL error reads
- shader info-log length and program info-log reads

`LLPostProcess` still owns effect selection, shader uniform ownership, texture
binding, formats, dimensions, matrix setup, and postprocess ordering.

## Inventory Result

Generated source inventory after the packet:

- `indra/llrender/llpostprocess.cpp`: active direct `gl*` calls dropped from
  13 to 0
- `indra/llrender/llglcontainment.cpp`: likely `gl*` call expressions rose
  from 50 to 59

The remaining raw references in `llpostprocess.cpp` are non-call references
reported by the generated inventory.

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

No Xcode or runtime smoke was run because this is pure wrapper routing.

## Next Candidate

Recommended next step:

- classify the remaining small `indra/llrender` direct-call entries before
  source edits

Reason:

- several entries may be generated-inventory false positives or intentionally
  low-level owners
- `llrender.cpp`, `llgl.cpp`, `llglslshader.cpp`, and `llshadermgr.cpp` are
  larger and should not be handled as blind mechanical packets
