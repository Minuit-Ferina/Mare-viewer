# Model Preview LOD Optimizer Options Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns reading the LOD optimizer controls used by
`LLModelPreview::genMeshOptimizerLODs()` through:

- `getModelPreviewLODMode(S32 lod, U32 default_mode)`
- `getModelPreviewLODTriangleLimit(S32 lod) const`
- `getModelPreviewLODErrorThresholdPercent(S32 lod) const`

`LLModelPreview::genMeshOptimizerLODs()` no longer directly reads:

- `lod_mode_*`
- `lod_triangle_limit_*`
- `lod_error_threshold_*`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- LOD range validation and empty-base-model checks before UI option reads;
- selected LOD mode deciding which numeric control is read;
- default triangle-limit calculation when `enforce_tri_limit` is false;
- error threshold conversion from percent to 0..1 inside `LLModelPreview`;
- model loading, upload, LOD generation, physics, status, and render behavior.

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

Remaining `LLModelPreview` UI reads include:

- model description and import scale during upload-data rebuild;
- direct child lookup for LOD status widgets in `updateLodControls()`;
- direct child lookup for scale spinner limits in `rebuildUploadData()`.

Remaining `LLModelPreview` UI mutations include:

- model load and LOD status controls;
- physics status controls;
- calculation and upload button enablement;
- physics decomposition controls.

The next packet should either handle the remaining upload-data scalar reads or
start a more deliberate LOD status-control ownership pass.
