# Model Preview Physics Decomposition Controls Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move physics decomposition panel and button synchronization used by
`LLModelPreview::updateStatusMessages()` behind a `LLFloaterModelPreview` owner
method without changing conditions.

## Current Behavior

`LLModelPreview::updateStatusMessages()` currently:

- shows the active physics analysis panels;
- enables or disables analysis panel children based on physics geometry and
  current decomposition requests;
- enables or disables simplification panel children based on hull availability
  and current requests;
- toggles `Simplify`, `Decompose`, `Analyze`, and their cancel buttons.

## Target Ownership

`LLFloaterModelPreview` should own decomposition panel and button UI state.

`LLModelPreview` should keep computing whether physics triangles or hulls are
available.

## Required Ordering

Preserve these constraints:

1. Physics totals and `show_physics` synchronization still happen first.
2. The same `mCurRequest.empty()` checks control panel and button state.
3. The same Havok/VHACD panel selection remains under the existing compile
   flag.
4. Physics LOD mode/file-control and crease-control synchronization still
   happen after decomposition controls.
5. No model loading, upload, LOD, physics generation, status, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change decomposition request behavior;
- change button names or visibility conditions;
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
