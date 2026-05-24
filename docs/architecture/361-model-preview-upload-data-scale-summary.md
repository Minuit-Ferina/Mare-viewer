# Model Preview Upload Data Scale Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns upload-data description and import-scale UI
access through:

- `getModelPreviewRequestedName() const`
- `getModelPreviewImportScale() const`
- `syncModelPreviewImportScaleLimit(F32 max_import_scale, F32 current_scale)`

`LLModelPreview::rebuildUploadData()` no longer directly reads or mutates:

- `description_form`
- `import_scale`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- description and import-scale reads before upload-data instance rebuild;
- scale-matrix construction from the same import-scale value;
- maximum import-scale computation inside `LLModelPreview`;
- import-scale spinner max and clamp updates after maximum scale is computed;
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

Remaining `LLModelPreview` UI access is now mostly mutation/status ownership:

- `calculate_btn` enable/disable during model-load and validation flows;
- upload and physics button enablement;
- LOD and physics status fields;
- LOD status widget lookups in `updateLodControls()`;
- default description initialization after model load.

The next packet should move a small mutation family behind floater-owned
methods, starting with the repeated `calculate_btn` disable calls or the
default description initialization.
