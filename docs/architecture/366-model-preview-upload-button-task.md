# Model Preview Upload Button Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move `ok_btn` enable/disable mutations used by `LLModelPreview` behind a
`LLFloaterModelPreview` owner method without changing any conditions.

## Current Behavior

`LLModelPreview` currently enables or disables `ok_btn` directly during:

- model-load error handling;
- upload status validation.

## Target Ownership

`LLFloaterModelPreview` should own mutating the upload button control.

`LLModelPreview` should keep all decisions about when upload is allowed.

## Required Ordering

Preserve these constraints:

1. The same validation and load-state conditions still decide button state.
2. Button mutations remain at the same points in the control flow.
3. Calculate button behavior is not changed in this packet.
4. No model loading, upload, LOD, physics, status, or render behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change upload button enablement conditions;
- change calculate button behavior;
- restructure validation logic;
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
