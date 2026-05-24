# Model Preview Skin UI Sync Task

Date: 2026-05-24

Branch: `phase11`

## Task

Move skin preview UI control synchronization ownership from `LLModelPreview` to
`LLFloaterModelPreview` without changing runtime behavior.

This is a boundary ownership packet, not a file reorganization.

## Current Owner

`LLModelPreview::updateSkinPreviewControls(...)` currently both:

- computes model/skin state needed by rendering;
- mutates floater UI controls.

## Target Owner

`LLModelPreview` should continue to compute:

- whether the current preview LOD has skin weights;
- pelvis offset assignment for preview models;
- render-time `upload_skin`, `upload_joints`, and `show_skin_weight` values.

`LLFloaterModelPreview` should apply:

- avatar tab clear/populate;
- skin and joint upload control values;
- skin/joint view option enablement;
- rig warning visibility;
- lock-scale control enablement;
- upload joint control enablement.

## Required Ordering

The packet must preserve this order:

1. `LLModelPreview::render()` reads current `upload_skin`, `upload_joints`, and
   `show_skin_weight` values.
2. `mLastJointUpdate` is compared and updated before any avatar tab refresh.
3. Preview models receive the current pelvis offset while the scene is scanned.
4. Skin weights are detected before any skin UI control changes.
5. Legacy rig flags are read before enabling skin weight view controls.
6. First skin update auto-enables `upload_skin` and `show_skin_weight` in the
   same conditions as before.
7. Missing skin weights still disable skin upload and skin/joint view options.
8. `upload_skin` and `upload_joints` out-parameters still reflect any forced
   control changes before camera setup and draw branch selection.
9. Avatar tab update/clear order stays unchanged.
10. Joint upload control enablement still depends on
    `isRigValidForJointPositionUpload()`.

## Explicit Non-Goals

Do not:

- move source files;
- move the UI sync timing out of render in this packet;
- change `needsRender()`;
- change dynamic texture order;
- change model upload or validation behavior;
- touch `pipeline.cpp`;
- touch broad `llui`.

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
