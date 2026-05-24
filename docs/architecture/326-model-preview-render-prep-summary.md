# Model Preview Render Prep Summary

Date: 2026-05-24

Branch: `phase10`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

`LLModelPreview::render()` now delegates three preparation blocks to private
owner-local helpers:

- `updateSkinPreviewControls(...)`
- `ensurePreviewLODVertexBuffers(...)`
- `applyPreviewMaterial(...)`

The helper split keeps the existing order:

1. canvas draw remains before upload and skin preview control handling;
2. upload skin and upload joints values are read before skin preview control
   handling;
3. skin-weight detection, pelvis offset assignment, upload control mutation,
   avatar tab update, and lock-scale control gating stay together;
4. physics explode is still read after skin preview controls;
5. camera, shader, and lighting setup still happen before buffer checks;
6. base model buffer generation remains before the preview LOD model block;
7. reset button enablement remains before LOD/physics buffer checks;
8. material diffuse color and texture binding still happen immediately before
   the same model buffer draw calls in both non-skinned and skinned paths.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not touch:

- model parsing, upload, or validation;
- LOD generation policy;
- physics generation or rendering;
- skinned avatar rendering;
- camera math;
- shader choices;
- edge overlay drawing;
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

Risk remains high for the rest of `LLModelPreview::render()`.

The method still contains large draw branches for ordinary model preview,
physics preview, degenerate triangle debug rendering, skinned avatar preview,
joint override setup, and bone/collision-volume display.

The next source packet should be bigger than a one-line wrapper but still stay
inside one coherent branch, such as:

- ordinary non-skinned model draw;
- physics preview draw;
- skinned avatar preview draw;
- camera setup.
