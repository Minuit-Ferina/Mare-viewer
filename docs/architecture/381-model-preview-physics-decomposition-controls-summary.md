# Model Preview Physics Decomposition Controls Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns physics decomposition control synchronization
through:

- `syncModelPreviewPhysicsDecompositionControls(bool has_physics_tris, bool has_physics_hulls)`

`LLModelPreview::updateStatusMessages()` no longer directly mutates:

- physics analysis panels;
- physics simplification panel children;
- `Simplify`, `simplify_cancel`;
- `Decompose`, `decompose_cancel`;
- `Analyze`, `analyze_cancel`.

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- physics totals and `show_physics` synchronization before decomposition
  controls;
- `mCurRequest.empty()` checks for analysis, simplification, and cancel buttons;
- existing Havok/VHACD panel selection;
- physics LOD mode/file-control and crease-control synchronization after
  decomposition controls;
- model loading, upload, LOD, physics generation, status, and render behavior.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

No broad `mare-viewer` integration build was run.

## Residual Risk

Remaining `LLModelPreview` UI access is now concentrated in:

- status text and icon updates for LOD and physics summaries;
- initial rig option mutations during model-load callback;
- preview panel rectangle access for rendering.

The next packet should target a status text/icon family, starting with physics
summary text because it is less entangled with LOD validation decisions.
