# GLTF Preview Render Split Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

This is the first phase 5 source packet after
`docs/architecture/261-gltf-preview-owner-map.md`.

The change is owner-local to `LLGLTFPreviewTexture`. It splits
`LLGLTFPreviewTexture::render()` into smaller helpers without changing material
load policy, UI entry points, shader selection, render target routing,
post-processing order, dynamic texture behavior, or cleanup policy.

## Files Changed

- `indra/newview/llgltfmaterialpreviewmgr.h`
- `indra/newview/llgltfmaterialpreviewmgr.cpp`
- `docs/architecture/generated/source_inventory.csv`

## Source Changes

Added private `LLGLTFPreviewTexture` helpers:

- `setupPreviewCamera(...)`
- `renderFinalPreview(...)`

Added owner-local anonymous namespace helpers:

- `get_preview_light_direction()`
- `get_transformed_light_direction(...)`
- `setup_preview_light(...)`
- `render_alpha_preview_sphere(...)`
- `run_preview_post_processing(...)`

The extraction preserves the original order:

1. guard `mShouldRender`;
2. clear transparent color and depth;
3. set temporary GL and pipeline state;
4. disable local lights;
5. force default reflection probe state;
6. set up preview camera and object transform;
7. get/update the preview sphere;
8. set up hardware lights and preview sun direction;
9. reset `gGLLastMatrix`;
10. render the alpha preview sphere into `gPipeline.mAuxillaryRT.screen`;
11. run the existing post-processing chain;
12. render the final processed image into `mBoundTarget`;
13. restore hardware lights, reflection probe state, and
    `RenderLocalLightCount`.

## Behavior Preserved

This packet does not change:

- `UIPreviewMaterial`;
- material load-level comparisons;
- `LLGLTFPreviewTexture` creation policy;
- `ORDER_MIDDLE`;
- preview sphere geometry or UVs;
- shader constants;
- shader programs;
- post-processing order;
- `mExposureMap` / `mLastExposure` swap timing;
- final `mBoundTarget` usage;
- `LLTextureCtrl` or `LLFloaterTexturePicker`;
- dynamic texture `preRender(...)` / `postRender(...)` behavior.

## Risk

Risk is medium.

Why:

- this owner touches pipeline render targets, shaders, post-processing, and
  dynamic texture copy-back;
- the diff is a local helper split with the same call order;
- the UI entry points and material load policy are untouched.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llgltfmaterialpreviewmgr.cpp.o -j8
```

Result: passed.

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- source inventory regenerated; scanned 3085 source files;
- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers do not include `llgl.h` directly outside PCH/prefix;
- raw GL scalar type names are confined to `llglheaders.h`;
- whitespace check passed.

## Integration Build

No non-clean `mare-viewer` integration checkpoint was run for this first phase
5 source packet.

Reason:

- the targeted owner build passed;
- both OpenGL guardrails passed;
- the packet only extracts local helpers and does not touch `LLTextureCtrl` or
  dynamic texture driver behavior;
- phase 5 should reserve integration builds for larger source milestones.

## Next Small Tasks

- Review whether `LLGLTFPreviewTexture::render()` needs one more owner-local
  state object for preview pipeline state.
- Keep `LLTextureCtrl` separate until the GLTF preview render owner is stable.
- Map `LLViewerTexLayerSetBuffer` separately before avatar bake cleanup.
