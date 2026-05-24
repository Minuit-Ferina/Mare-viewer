# Model Preview Physics File Controls Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move physics LOD combo reads and physics file-control enablement used by
`LLModelPreview::updateStatusMessages()` behind `LLFloaterModelPreview` owner
methods without changing any conditions.

## Current Behavior

`LLModelPreview::updateStatusMessages()` currently:

- reads `physics_lod_combo`;
- derives the selected mode and file-mode index;
- enables `physics_file` and `physics_browse` when file mode is selected;
- disables those controls otherwise.

## Target Ownership

`LLFloaterModelPreview` should own reading and mutating physics file controls.

`LLModelPreview` should keep the decision about whether the selected physics
LOD mode is file mode.

## Required Ordering

Preserve these constraints:

1. Physics status and decomposition controls still update before physics file
   controls.
2. Missing physics LOD combo preserves the existing default `which_mode` and
   `file_mode` values.
3. The same equality check still decides whether file controls are enabled.
4. No model loading, upload, LOD, physics generation, status, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change physics LOD mode semantics;
- change physics decomposition panel behavior;
- change show-physics view option behavior;
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
