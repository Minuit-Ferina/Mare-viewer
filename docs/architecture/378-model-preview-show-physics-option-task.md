# Model Preview Show Physics Option Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move `show_physics` view option synchronization used by
`LLModelPreview::updateStatusMessages()` behind a `LLFloaterModelPreview` owner
method without changing conditions.

## Current Behavior

`LLModelPreview::updateStatusMessages()` currently:

- enables `show_physics` only when physics triangles or hulls exist and the
  control is currently disabled;
- updates `mViewOption["show_physics"]` to true only in that enable path;
- disables `show_physics` and sets the UI value false when no physics triangles
  or hulls exist;
- updates `mViewOption["show_physics"]` to false in that disable path.

## Target Ownership

`LLFloaterModelPreview` should own mutating the `show_physics` control.

`LLModelPreview` should keep the computed physics availability and retain
ownership of the preview option map.

## Required Ordering

Preserve these constraints:

1. Physics triangle and hull totals are computed before `show_physics`
   synchronization.
2. The same physics-availability condition controls enable/disable behavior.
3. The preview option map is updated only on the same paths as before.
4. No model loading, upload, LOD, physics generation, status, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change view option semantics;
- change physics decomposition panel behavior;
- change physics status text or icon updates;
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
