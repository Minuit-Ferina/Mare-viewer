# Model Preview Selected LOD Sync Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns selected preview LOD synchronization through:

- `syncModelPreviewSelectedLOD(S32 lod, const std::string& filename)`

`LLModelPreview::setPreviewLOD()` no longer directly accesses:

- `preview_lod_combo`
- `lod_file_*`
- LOD row label/status/triangle/vertex colors

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- `mPreviewLOD` mutation before UI synchronization;
- reversed LOD combo index selection;
- LOD file and row color synchronization before avatar tab clearing;
- avatar tab clearing, refresh, and status update order;
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

- show-physics view option synchronization;
- physics decomposition panel and button visibility;
- status text and icon updates for LOD and physics summaries.

The next packet should target a narrow physics panel ownership family or start
grouping LOD/physics status text updates behind floater-owned methods.
