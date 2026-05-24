# Model Preview Dimension Options Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move the dimension/import-scale UI reads used by
`LLModelPreview::updateDimentionsAndOffsets()` behind a
`LLFloaterModelPreview` owner method without changing timing.

## Current Behavior

`LLModelPreview::updateDimentionsAndOffsets()` currently reads:

- `pelvis_offset`
- `upload_joints`
- `import_scale`

It uses those values to:

- set `mPelvisZOffset`;
- decide whether to add the preview avatar pelvis fixup;
- update LOD skin info pelvis offsets;
- report scaled import dimensions through `mDetailsSignal`.

## Target Ownership

`LLFloaterModelPreview` should own reading the UI controls.

`LLModelPreview` should keep applying the values to model and preview-avatar
state.

## Required Ordering

Preserve these constraints:

1. `rebuildUploadData()` still runs before reading these values.
2. Default values with no floater stay unchanged:
   - pelvis offset `3.0f`;
   - upload joints `false`;
   - import scale `1.0f`, producing the existing `2.0f` multiplier.
3. Pelvis fixup still uses the current pelvis offset and current upload-joints
   value.
4. LOD skin info pelvis offsets are updated after `mPelvisZOffset`.
5. `mDetailsSignal` still receives dimensions multiplied by `import_scale * 2`.
6. `updateStatusMessages()` still runs last.

## Explicit Non-Goals

Do not:

- move source files;
- change spelling or broader structure of `updateDimentionsAndOffsets()`;
- change import-scale math;
- change upload, validation, LOD, physics, or render behavior;
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
