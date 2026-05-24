# Model Preview Camera Summary

Date: 2026-05-24

Branch: `phase10`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

`LLModelPreview::render()` now delegates 3D preview camera, shader, and light
setup to:

- `setupPreviewCamera(bool show_skin_weight, S32 width, S32 height)`

The helper returns a private `PreviewCameraState` containing:

- `offset`
- `target_pos`
- `av_rot`
- `camera_distance`

The returned state preserves the existing skinned preview camera recentering
that happens before joint override preview work.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet preserves:

- preview panel aspect calculation;
- viewer camera FOV setup;
- skinned preview target switch;
- skinned preview `refresh()` timing;
- `gObjectPreviewProgram` bind timing;
- modelview identity load;
- preview light enablement;
- camera rotation, distance, origin, look-at, near/far, and perspective math;
- `stop_glerror()` placement before model matrix push;
- skinned preview camera recentering before joint override work.

This packet does not touch:

- model, physics, skinned avatar, joint, or debug drawing;
- upload or validation behavior;
- LOD generation policy;
- shader choices;
- dynamic texture order or target behavior;
- `pipeline.cpp` or broad `llui`.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Risk remains high in the remaining draw branches.

The next packets can be larger than previous single-helper changes, but should
still keep coherent ownership boundaries:

- ordinary non-skinned model draw;
- physics preview draw;
- skinned avatar and joint preview draw.
