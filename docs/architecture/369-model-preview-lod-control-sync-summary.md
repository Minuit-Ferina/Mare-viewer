# Model Preview LOD Control Sync Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns LOD control reads and widget synchronization
through:

- `getModelPreviewLODSourceMode(S32 lod)`
- `setModelPreviewLODMode(S32 lod, S32 mode)`
- `syncModelPreviewLODFileControls(S32 lod, bool visible)`
- `syncModelPreviewLODGenerateControlsVisible(S32 lod, bool visible)`
- `syncModelPreviewLODGenerateControls(S32 lod, U32 max_triangle_limit, S32 requested_triangle_count, F32 requested_error_threshold, U32 requested_lod_mode)`

`LLModelPreview::updateLodControls()` no longer directly accesses:

- `lod_source_*`
- `lod_browse_*`
- `lod_file_*`
- `lod_mode_*`
- `lod_triangle_limit_*`
- `lod_error_threshold_*`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- LOD range validation before UI access;
- early return when the LOD source combo is missing;
- file, use-above, and generated LOD paths;
- recursive lower-LOD updates;
- generated control synchronization while `mLODFrozen` is true;
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

Remaining `LLModelPreview` UI access is now concentrated in:

- physics control visibility and enablement;
- physics LOD combo ownership;
- status text and icon updates for LOD and physics summaries;
- model-load file field updates.

The next packet should target the small physics LOD combo/file-control family
before attempting larger status text/icon ownership.
