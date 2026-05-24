# Model Preview Physics File Controls Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLFloaterModelPreview` now owns physics file-control access through:

- `getModelPreviewPhysicsLODMode(S32& which_mode, S32& file_mode)`
- `syncModelPreviewPhysicsFileControls(bool enabled)`

`LLModelPreview::updateStatusMessages()` no longer directly accesses:

- `physics_lod_combo`
- `physics_file`
- `physics_browse`

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- physics status and decomposition controls before physics file-control sync;
- default `which_mode` and `file_mode` values when the combo is missing;
- the same file-mode equality check for enablement;
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

- show-physics view option synchronization;
- physics decomposition panel and button visibility;
- status text and icon updates for LOD and physics summaries;
- model-load file field updates.

The next packet should target the crease control synchronization or a small
model-load file-field update family.
