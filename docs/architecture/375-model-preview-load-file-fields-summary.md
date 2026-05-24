# Model Preview Load File Fields Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns load status and file-field mutation through:

- `setModelPreviewReadingFileStatus()`
- `setModelPreviewLODFile(S32 lod, const std::string& filename)`
- `setModelPreviewPhysicsFile(const std::string& filename)`

`LLModelPreview` no longer directly mutates:

- `status` with `status_reading_file`
- `lod_file_*`
- `physics_file`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- reading-file status update immediately after the model loader starts;
- existing branch conditions for LOD and physics file fields;
- physics-from-LOD clearing of the physics filename before upload-data rebuild;
- preview LOD file update when preview LOD changes;
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

- preview LOD combo selection and row highlight colors;
- show-physics view option synchronization;
- physics decomposition panel and button visibility;
- status text and icon updates for LOD and physics summaries.

The next packet should target preview LOD selection/highlighting or keep that
for a separate visible-state ownership pass.
