# Model Preview Upload Status Options Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move the upload option UI reads used by `LLModelPreview::updateStatusMessages()`
behind a `LLFloaterModelPreview` owner method without changing timing.

## Current Behavior

`LLModelPreview::updateStatusMessages()` currently reads:

- `upload_skin`
- `upload_joints`
- `upload_textures`

It uses those values to:

- reject joint-position upload when the rig is invalid;
- reject upload while requested textures are still fetching.

## Target Ownership

`LLFloaterModelPreview` should own reading upload option controls.

`LLModelPreview` should keep the validation decisions based on those values.

## Required Ordering

Preserve these constraints:

1. Loader error handling still runs before upload option checks.
2. Upload skin/joint options are checked before texture readiness.
3. Joint-position validation still depends on
   `isRigValidForJointPositionUpload()`.
4. Texture readiness still only blocks upload when texture upload is selected.
5. No status text, icon, model loading, upload, LOD, physics, or render
   behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change upload validation semantics;
- change texture fetching or upload behavior;
- change status UI text or icon updates;
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
