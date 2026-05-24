# Model Preview Skin UI Sync Summary

Date: 2026-05-24

Branch: `phase11`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLModelPreview::updateSkinPreviewControls(...)` now keeps the model-side
work:

- scan the current preview LOD scene;
- assign pelvis offset to preview models;
- detect whether any preview model has skin weights;
- pass the current render-time skin/joint booleans through to the floater sync.

`LLFloaterModelPreview::syncSkinPreviewControls(...)` now owns the skin preview
UI mutations:

- avatar tab clear/populate;
- `upload_skin` and `upload_joints` value synchronization;
- skin/joint view option enablement;
- rig warning visibility;
- `lock_scale_if_joint_position` enablement and reset;
- `upload_joints` enablement after joint-upload validation.

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- the existing render-time call site;
- `mLastJointUpdate` comparison order;
- preview scene scan order;
- first skin update auto-enable behavior;
- forced `upload_skin` and `upload_joints` value changes;
- avatar tab update/clear order;
- joint upload control validation order;
- dynamic texture and render order.

This packet does not fully remove render-time UI synchronization. It changes
ownership first: UI controls are now mutated by the floater class instead of
directly by the model preview class.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

The regenerated inventory reflects the ownership shift:

- `indra/newview/llmodelpreview.cpp` line count decreased;
- `indra/newview/llfloatermodelpreview.cpp` line count increased;
- OpenGL containment counts did not regress.

## Residual Risk

Remaining UI/render coupling:

- `LLModelPreview::render()` still reads upload controls directly;
- `LLModelPreview::render()` still triggers the skin UI synchronization timing;
- `LLModelPreview::render()` still enables `reset_btn`;
- other model preview update paths still mutate floater controls outside the
  render helper split.

The next low-risk packet is to map and move the `reset_btn` render-path
mutation to `LLFloaterModelPreview::draw()` or another floater-owned UI sync
point without changing reset behavior.
