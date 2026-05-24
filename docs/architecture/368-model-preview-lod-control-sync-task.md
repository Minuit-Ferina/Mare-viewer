# Model Preview LOD Control Sync Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move `LLModelPreview::updateLodControls()` LOD control reads and widget
synchronization behind `LLFloaterModelPreview` owner methods without changing
LOD selection or generation behavior.

## Current Behavior

`LLModelPreview::updateLodControls()` currently accesses:

- `lod_source_*`
- `lod_browse_*`
- `lod_file_*`
- `lod_mode_*`
- `lod_triangle_limit_*`
- `lod_error_threshold_*`

It uses those controls to:

- read the source mode for a requested LOD;
- show file controls when LOD is loaded from file;
- hide file and generated controls when using the LOD above;
- show and synchronize generated LOD controls for meshoptimizer mode.

## Target Ownership

`LLFloaterModelPreview` should own reading and mutating LOD controls.

`LLModelPreview` should keep:

- LOD index validation;
- decisions about source-mode behavior;
- model, scene, and vertex buffer mutation;
- recursive lower-LOD updates;
- `mLODFrozen` timing.

## Required Ordering

Preserve these constraints:

1. LOD range validation still happens before UI access.
2. Missing source combo still returns without changing model state.
3. Source mode still decides the same file, above-LOD, or generated path.
4. Generated control synchronization still happens while `mLODFrozen` is true.
5. No model loading, upload, LOD generation, physics, status, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change LOD source mode semantics;
- change generated LOD control values;
- change recursive LOD update behavior;
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
