# Model Preview Render Option Read Summary

Date: 2026-05-24

Branch: `phase11`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns reading the upload/physics controls needed by
the current model preview render frame:

- `upload_skin`
- `upload_joints`
- `physics_explode`

The new owner method is:

- `getModelPreviewRenderOptions(...)`

`LLModelPreview::render()` no longer directly calls:

- `mFMP->childGetValue("upload_skin")`
- `mFMP->childGetValue("upload_joints")`
- `mFMP->childGetValue("physics_explode")`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- reading upload/physics controls before skin UI sync;
- allowing skin UI sync to adjust `upload_skin`, `upload_joints`, and
  `show_skin_weight` before camera setup and draw branch selection;
- capturing `physics_explode` before physics preview drawing;
- dynamic texture and render order;
- model upload, LOD, physics, and validation behavior.

This does not cache values across frames. It only moves control-reading
ownership to the floater while preserving the current render-time timing.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Remaining model preview UI/render coupling:

- render-time skin UI synchronization still happens through a floater-owned
  method;
- `LLModelPreview::render()` still receives UI-derived values every render;
- `LLFloaterModelPreview::draw3dPreview()` still contains direct draw work;
- broader model preview load/update paths still contain UI mutation.

The next useful step is to decide whether render-time skin UI synchronization
can move to a pending-state application point in `LLFloaterModelPreview`
without creating a frame-order regression.
