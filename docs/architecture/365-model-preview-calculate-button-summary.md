# Model Preview Calculate Button Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns calculate button mutation through:

- `setModelPreviewCalculateButtonEnabled(bool enabled)`

`LLModelPreview` no longer directly enables or disables:

- `calculate_btn`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- all existing validation and load-state conditions;
- button mutations at the same control-flow points;
- `ok_btn` behavior;
- model loading, upload, LOD, physics, status, and render behavior.

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

- upload button enable/disable;
- LOD and physics status text/icon updates;
- LOD control visibility and spinner bounds;
- physics control visibility and enablement;
- physics LOD combo ownership.

The next packet can target `ok_btn` enable/disable or keep button ownership for
a separate action-button pass.
