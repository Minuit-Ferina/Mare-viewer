# Model Preview Upload Button Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns upload button mutation through:

- `setModelPreviewUploadButtonEnabled(bool enabled)`

`LLModelPreview` no longer directly enables or disables:

- `ok_btn`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- all existing validation and load-state conditions;
- button mutations at the same control-flow points;
- calculate button behavior;
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

- status text and icon updates;
- LOD control visibility and spinner bounds;
- physics control visibility and enablement;
- physics LOD combo ownership.

The next packet should target either the model-load file/status fields or the
LOD control status widgets, depending on which can be kept smallest.
