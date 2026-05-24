# Model Preview Render Option Read Task

Date: 2026-05-24

Branch: `phase11`

## Task

Move the remaining direct render-time UI control reads in
`LLModelPreview::render()` behind a `LLFloaterModelPreview` owner method without
changing when those values are read.

## Current Behavior

`LLModelPreview::render()` currently reads these controls directly:

- `upload_skin`
- `upload_joints`
- `physics_explode`

The render method also reads `mViewOption` values. Those are model preview
render options and are not changed by this packet.

## Target Ownership

`LLFloaterModelPreview` should own reading upload/physics UI controls.

`LLModelPreview::render()` should request the values it needs for the current
frame and continue using them in the same order.

## Required Ordering

Preserve these constraints:

1. `show_*` values are read from `mViewOption` first, as before.
2. Upload and physics UI controls are read before skin preview synchronization
   and camera setup, as before.
3. Skin UI synchronization can still adjust `upload_skin`, `upload_joints`, and
   `show_skin_weight` before draw branch selection.
4. `physics_explode` is still captured before physics preview drawing.
5. No dynamic texture update, render branch, model load, LOD, or physics
   behavior changes.

## Explicit Non-Goals

Do not:

- cache these values across frames yet;
- move source files;
- change `needsRender()`;
- change dynamic texture order;
- touch `pipeline.cpp`;
- touch broad `llui`;
- change model upload behavior.

## Verification

Run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
