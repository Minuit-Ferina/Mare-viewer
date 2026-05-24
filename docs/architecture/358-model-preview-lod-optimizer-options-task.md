# Model Preview LOD Optimizer Options Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move the LOD optimizer option UI reads used by
`LLModelPreview::genMeshOptimizerLODs()` behind `LLFloaterModelPreview` owner
methods without changing timing.

## Current Behavior

`LLModelPreview::genMeshOptimizerLODs()` currently reads:

- `lod_mode_*`
- `lod_triangle_limit_*`
- `lod_error_threshold_*`

It uses those values to:

- choose triangle-limit mode or error-threshold mode for a requested LOD;
- convert a requested triangle limit into the meshoptimizer decimation ratio;
- convert the UI error threshold percentage into the meshoptimizer 0..1 range.

## Target Ownership

`LLFloaterModelPreview` should own reading LOD optimizer controls.

`LLModelPreview` should keep LOD generation, meshoptimizer selection, ratio
math, logging, and model mutation based on those values.

## Required Ordering

Preserve these constraints:

1. LOD range validation and empty-base-model checks still run before reading
   LOD optimizer controls.
2. The selected LOD mode still decides whether the triangle limit or error
   threshold control is read.
3. Default triangle-limit calculation still runs when `enforce_tri_limit` is
   false.
4. Error threshold conversion from percent to 0..1 still remains in
   `LLModelPreview`.
5. No model loading, upload, LOD generation, physics, status, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change meshoptimizer semantics;
- change LOD UI controls or callbacks;
- change triangle limit or error threshold math;
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
