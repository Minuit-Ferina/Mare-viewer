# Model Preview Load File Fields Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move model-load status and file-field mutations used by `LLModelPreview`
behind `LLFloaterModelPreview` owner methods without changing conditions.

## Current Behavior

`LLModelPreview` currently mutates:

- `status` with `status_reading_file`;
- `lod_file_*`;
- `physics_file`.

It updates those fields during:

- model loading;
- physics-from-LOD selection;
- preview LOD selection refresh.

## Target Ownership

`LLFloaterModelPreview` should own status and file-field UI mutation.

`LLModelPreview` should keep deciding when a load is in progress, which LOD is
being loaded, and which filename should be displayed.

## Required Ordering

Preserve these constraints:

1. Reading-file status still updates immediately after the model loader starts.
2. LOD and physics file fields still update under the same branch conditions.
3. Physics-from-LOD still clears the physics filename before rebuilding data.
4. Preview LOD file field still updates when preview LOD changes.
5. No model loading, upload, LOD, physics, status, or render behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change filename selection;
- change load-state transitions;
- change status string keys;
- touch preview LOD combo selection or row highlighting;
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
