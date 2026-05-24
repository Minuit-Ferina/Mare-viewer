# Model Preview Physics Summary Text Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move physics summary text synchronization used by
`LLModelPreview::updateStatusMessages()` behind a `LLFloaterModelPreview` owner
method without changing values.

## Current Behavior

`LLModelPreview::updateStatusMessages()` currently:

- writes `physics_triangles` with the triangle count or `mesh_status_na`;
- writes `physics_hulls` with the hull count or `mesh_status_na`;
- writes `physics_points` with the point count or `mesh_status_na`.

## Target Ownership

`LLFloaterModelPreview` should own physics summary text UI mutation.

`LLModelPreview` should keep computing physics triangle, hull, and point
counts.

## Required Ordering

Preserve these constraints:

1. Physics counts are computed before summary text synchronization.
2. Positive triangle count still controls the triangle text.
3. Positive hull count still controls both hull and point text.
4. The same `mesh_status_na` fallback text is used.
5. No model loading, upload, LOD, physics generation, status, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change physics count computation;
- change status string keys;
- change physics panel or button behavior;
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
