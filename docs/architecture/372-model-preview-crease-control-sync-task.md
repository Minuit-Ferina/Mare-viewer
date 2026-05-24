# Model Preview Crease Control Sync Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move crease control synchronization used by `LLModelPreview::updateStatusMessages()`
behind a `LLFloaterModelPreview` owner method without changing behavior.

## Current Behavior

`LLModelPreview::updateStatusMessages()` currently:

- gets the `crease_angle` spinner;
- sets `crease_label` grey and the spinner value to `75.f` when the current
  preview LOD has no requested crease angle;
- sets `crease_label` white and the spinner value to the requested crease angle
  otherwise.

## Target Ownership

`LLFloaterModelPreview` should own mutating the crease label and spinner.

`LLModelPreview` should keep selecting the requested crease-angle value for the
current preview LOD.

## Required Ordering

Preserve these constraints:

1. Crease control synchronization still happens after physics file-control sync.
2. `-1.f` still maps to grey label and `75.f`.
3. Non-`-1.f` values still map to white label and the requested value.
4. No model loading, upload, LOD, physics, status, or render behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change crease angle defaults;
- change normal generation;
- change physics control behavior;
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
