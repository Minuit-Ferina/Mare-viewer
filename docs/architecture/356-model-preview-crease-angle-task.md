# Model Preview Crease Angle Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move the `crease_angle` UI read used by `LLModelPreview::generateNormals()`
behind a `LLFloaterModelPreview` owner method without changing timing.

## Current Behavior

`LLModelPreview::generateNormals()` currently reads `crease_angle` directly
from the floater control and uses it to:

- store `mRequestedCreaseAngle` for the current preview LOD;
- convert the value from degrees to radians;
- regenerate normals for the selected preview LOD.

## Target Ownership

`LLFloaterModelPreview` should own reading the crease angle control.

`LLModelPreview` should keep normal generation and LOD data mutation based on
that value.

## Required Ordering

Preserve these constraints:

1. LOD bounds and empty-model checks still run before reading `crease_angle`.
2. `mRequestedCreaseAngle` is still stored before degree-to-radian conversion.
3. Normal generation still uses the same numeric value and conversion.
4. No model loading, upload, LOD, physics, status, or render behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change normal generation semantics;
- change the `crease_angle` UI control;
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
