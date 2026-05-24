# Model Preview Upload Data Scale Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move the upload-data description and import-scale UI access used by
`LLModelPreview::rebuildUploadData()` behind `LLFloaterModelPreview` owner
methods without changing timing.

## Current Behavior

`LLModelPreview::rebuildUploadData()` currently reads:

- `description_form`
- `import_scale`

It also mutates the import-scale spinner by:

- setting the computed maximum import scale;
- clamping the current import scale when the computed maximum is smaller.

## Target Ownership

`LLFloaterModelPreview` should own reading and mutating floater controls.

`LLModelPreview` should keep computing upload data, model labels, scale
matrices, maximum import scale, and bounding-box limits.

## Required Ordering

Preserve these constraints:

1. Description and import scale are read before upload-data instances are
   rebuilt.
2. The scale matrix still uses the same import-scale value.
3. Maximum import scale is still computed after upload-data and bounds work.
4. Import-scale spinner max and clamp updates still happen after the maximum
   value is computed.
5. No model loading, upload, LOD, physics, status, or render behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change import-scale math;
- change model label assignment;
- change spinner limits or clamping behavior;
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
