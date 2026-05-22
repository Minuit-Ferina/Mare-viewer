# Dynamic Texture Render Scope Task

Date: 2026-05-22

## Scope

This is a future source task note for `LLViewerDynamicTexture`.

Do not implement this task until a focused source packet is explicitly chosen.
The purpose is to constrain any future extraction around
`LLViewerDynamicTexture::updateAllInstances()`.

## Owner File

- `indra/newview/lldynamictexture.cpp`

Potentially affected header only if a private class/member helper is required:

- `indra/newview/lldynamictexture.h`

Prefer an implementation-local helper in `lldynamictexture.cpp` if possible.

## Candidate Behavior

The candidate is a behavior-preserving local render scope/helper for the dynamic
texture update path.

It may clarify:

- render target binding and flush order;
- per-instance bound-target assignment and clearing;
- viewport/camera setup and restoration through existing virtual calls;
- depth clear behavior before each instance render;
- shader and vertex-buffer unbind points;
- separation between preview target users and bake target users.

It must not change:

- order bucket iteration;
- target selection;
- virtual call order;
- camera restoration;
- viewport restoration;
- render result counting;
- `LLViewerTexLayerSetBuffer` bake behavior;
- `LLVisualParamReset` behavior;
- `LLPreviewAnimation` refresh behavior.

## Existing Flow To Preserve

Preview target group:

- target: `gPipeline.mAuxillaryRT.deferredScreen`
- orders: `ORDER_FIRST` through before `ORDER_LAST`
- expected size: `LLPipeline::MAX_PREVIEW_WIDTH`

Bake target group:

- target: `gPipeline.mBakeMap`
- orders: `ORDER_LAST` through before `ORDER_COUNT`
- expected size:
  - `LLAvatarAppearanceDefines::SCRATCH_TEX_WIDTH`
  - `LLAvatarAppearanceDefines::SCRATCH_TEX_HEIGHT`

Per instance:

1. Check `needsRender()`.
2. Assert the dynamic texture fits in the selected target.
3. Clear depth.
4. Set immediate color to white.
5. Assign `mBoundTarget` through `setBoundTarget(&renderTarget)`.
6. Call `preRender()`.
7. Call virtual `render()`.
8. If `render()` returns true, set the local result and increment
   `sNumRenders`.
9. Flush `gGL`.
10. Unbind `LLVertexBuffer`.
11. Clear the bound target with `setBoundTarget(nullptr)`.
12. Call `postRender(result)`.

## Important Existing Return Semantics

Current source behavior resets `ret` to `false` before the bake-target group.
That means the final return value reflects the bake-target group, not the
preview-target group.

Do not accidentally change this during a mechanical cleanup.

If this is a bug, it should be fixed as a separate behavior change with a
specific validation plan. It should not be hidden inside a render-scope
refactor.

## Special Users

Handle these users as special cases in review:

- `LLViewerTexLayerSetBuffer`
  - `ORDER_LAST`
  - uses `gPipeline.mBakeMap`
  - routes rendering through `renderTexLayerSet(mBoundTarget)`
- `LLVisualParamReset`
  - `ORDER_RESET`
  - uses base `needsRender()`
  - `render()` can mutate avatar state but returns `false`
- `LLPreviewAnimation`
  - has `needsUpdate()`, but no `needsRender()` override found
  - likely uses base `needsRender()`

## Verification Plan

For a source patch:

```sh
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Manual runtime validation is recommended only if the source patch changes more
than local helper shape. Useful surfaces:

- login page startup;
- loaded scene with dynamic textures enabled;
- appearance editor visual parameter hints;
- avatar bake/composite refresh;
- model/image/BVH upload previews;
- GLTF material texture preview.

## Stop Conditions

Stop before source changes if the intended patch would:

- alter return semantics;
- reorder preview and bake groups;
- change `LLViewerTexLayerSetBuffer`'s `mBoundTarget` handoff;
- change `LLVisualParamReset` from a non-rendering reset path into a rendering
  result;
- require broad changes in `LLViewerDisplay`, `LLPipeline`, or UI preview
  classes.
