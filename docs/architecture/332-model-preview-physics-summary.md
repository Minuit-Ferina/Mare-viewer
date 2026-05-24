# Model Preview Physics Summary

Date: 2026-05-24

Branch: `phase10`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

`LLModelPreview::render()` now delegates physics preview drawing to:

- `renderPhysicsPreview(F32 physics_explode)`

The helper keeps the existing order:

1. the caller remains inside the `!show_skin_weight` branch and behind the
   existing `show_physics` gate;
2. depth is cleared before the two-pass physics draw loop;
3. pass 0 still writes depth only by disabling color writes;
4. pass 1 still restores color writes;
5. blend state and blend function are set inside the same pass scope;
6. physics hull/mesh selection remains under the same decomposition mutex;
7. physics hull mesh build and exploded hull draw order is unchanged;
8. physics mesh fill and edge draw order is unchanged;
9. degenerate triangle debug rendering remains after each physics pass;
10. line width, point size, preview lights, and scene blend type are restored in
    the same order.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not touch:

- physics decomposition logic;
- hull versus mesh selection;
- physics explode math;
- color-mask or blend order;
- degenerate triangle detection or draw state;
- ordinary model draw;
- skinned avatar preview drawing;
- upload or validation behavior;
- dynamic texture order or target behavior.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

The skinned avatar preview branch remains inside `LLModelPreview::render()`.

The next useful boundary is to split skinned avatar preview drawing as one
coherent helper, then reassess whether phase 10 has enough structure to move
from helper extraction toward separating UI mutation from render work.
