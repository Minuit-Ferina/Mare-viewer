# Model Preview Selected LOD Sync Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move selected preview LOD combo and row-highlight synchronization used by
`LLModelPreview::setPreviewLOD()` behind a `LLFloaterModelPreview` owner method
without changing behavior.

## Current Behavior

`LLModelPreview::setPreviewLOD()` currently:

- clamps and stores `mPreviewLOD`;
- updates `preview_lod_combo`;
- updates the selected `lod_file_*` field;
- highlights the selected LOD row and restores normal colors for the others;
- clears the avatar tab before refresh/status updates.

## Target Ownership

`LLFloaterModelPreview` should own preview LOD combo, file field, and row color
synchronization.

`LLModelPreview` should keep `mPreviewLOD` ownership, refresh timing, and status
updates.

## Required Ordering

Preserve these constraints:

1. `mPreviewLOD` still changes before UI synchronization.
2. Combo selection still uses the reversed LOD list index.
3. LOD file and row colors still update before avatar tab clearing.
4. Avatar tab clearing, refresh, and status updates keep their existing order.
5. No model loading, upload, LOD generation, physics, status, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change preview LOD clamping;
- change row color names;
- change avatar tab refresh behavior;
- touch `pipeline.cpp`;
- run a broad `mare-viewer` integration build.

## Verification

Run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
