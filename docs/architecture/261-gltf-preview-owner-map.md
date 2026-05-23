# GLTF Preview Owner Map

Date: 2026-05-23

Branch: `phase5`

## Scope

This note maps `LLGLTFPreviewTexture` before any phase 5 source changes.

The goal is to isolate the material-preview UI/render bridge without changing
runtime behavior. This owner is a `LLViewerDynamicTexture` user and renders a
GLTF material preview into a pipeline-owned target before the dynamic texture
driver copies the result into a viewer texture.

## Inputs Inspected

- `docs/architecture/206-ui-render-boundaries-post-containment.md`
- `docs/architecture/207-dynamic-texture-update-flow.md`
- `docs/architecture/208-dynamic-texture-user-table.md`
- `docs/architecture/209-dynamic-texture-overrides.md`
- `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`
- `docs/architecture/258-dynamic-texture-pass-split-summary.md`
- `indra/newview/llgltfmaterialpreviewmgr.h`
- `indra/newview/llgltfmaterialpreviewmgr.cpp`
- `indra/newview/lltexturectrl.cpp`
- `docs/architecture/generated/source_inventory.csv`

## Inventory Snapshot

Current generated inventory signals:

| file | category | `gGL` | `LLGL` | `LLPipeline` | `LLRenderTarget` | `LLViewerTexture` |
|---|---|---:|---:|---:|---:|---:|
| `indra/newview/llgltfmaterialpreviewmgr.cpp` | `render.pipeline` | 2 | 48 | 7 | 1 | 4 |
| `indra/newview/llgltfmaterialpreviewmgr.h` | `assets.texture` | 0 | 6 | 0 | 0 | 1 |

There are no direct runtime `gl*` calls in this owner. The remaining boundary
is higher-level render ownership through `LLGLContainment`, `LLGL*` state
objects, `gGL`, `LLPipeline`, shaders, render targets, and viewer textures.

## Owner Boundary

`LLGLTFMaterialPreviewMgr` owns:

- choosing whether the UI should receive a base-color texture or a dynamic
  material preview;
- rejecting previews for null or not-yet-loaded materials;
- honoring `UIPreviewMaterial`;
- creating uncached `LLGLTFPreviewTexture` instances.

`LLGLTFPreviewTexture` owns:

- dynamic texture participation through `ORDER_MIDDLE`;
- material load-level tracking;
- deciding whether a preview needs another render;
- delegating default dynamic-texture viewport/camera setup to
  `LLViewerDynamicTexture`;
- rendering the GLTF material on a preview sphere;
- using the auxiliary pipeline target pack during the preview render;
- post-processing the preview result;
- final draw into the dynamic texture bound target;
- clearing `mShouldRender` after `postRender()`.

It does not own:

- UI widget layout;
- texture picker fallback drawing;
- avatar bake behavior;
- `LLViewerDynamicTexture` target pass ordering;
- shader implementation;
- pipeline render target allocation;
- global material fetch policy outside material preview.

## UI Entry Points

Known UI callers are in `indra/newview/lltexturectrl.cpp`:

- `LLFloaterTexturePicker::draw()`;
- `LLTextureCtrl::draw()`.

Both paths:

- resolve material IDs through `gGLTFMaterialList`;
- request a preview through `gGLTFMaterialPreviewMgr.getPreview(...)`;
- cache the returned `mGLTFPreview` pointer locally;
- fall back to ordinary texture preview behavior when the material preview is
  unavailable;
- draw the resulting preview with UI image helpers.

Do not mix UI entry-point cleanup with GLTF preview rendering cleanup unless a
task explicitly names both surfaces.

## `getPreview(...)` Flow

`LLGLTFMaterialPreviewMgr::getPreview(...)` currently:

1. Returns null for a null material.
2. Reads `UIPreviewMaterial`.
3. If `UIPreviewMaterial` is false:
   - fetches the base-color texture for UI;
   - returns `material->mBaseColorTexture`.
4. If `UIPreviewMaterial` is true:
   - rejects materials that are still fetching;
   - fetches and checks all material texture slots;
   - returns null while any needed texture is not loaded enough;
   - creates a new `LLGLTFPreviewTexture` for the material.

Important note:

- the manager does not cache previews; callers are expected to cache when they
  ask for the same material repeatedly.

## Dynamic Texture Flow

`LLGLTFPreviewTexture` is constructed as:

- width: `LLPipeline::MAX_PREVIEW_WIDTH`;
- height: same as width;
- components: 4;
- order: `ORDER_MIDDLE`;
- clamp: false.

The phase 4 dynamic texture driver renders it in the preview-target pass, not
the avatar bake pass.

Virtual methods:

- `needsRender()` tracks material load improvement and `mShouldRender`;
- `preRender(...)` asserts and guards `mShouldRender`, then delegates to the
  base dynamic texture;
- `render()` owns the material preview render;
- `postRender(...)` clears `mShouldRender`, then delegates copy-back to the
  base dynamic texture.

## `render()` Responsibilities

`LLGLTFPreviewTexture::render()` currently does all of these in one method:

- clears the target with transparent black;
- disables or overrides depth, stencil, scissor, DoF, glow, SSR, and FSAA;
- temporarily points `gPipeline.mRT` at `gPipeline.mAuxillaryRT`;
- forces local light count to zero;
- updates default reflection probes;
- creates a local preview camera;
- builds or reuses the static preview sphere;
- updates GLTF material and vertex color on that sphere;
- sets up hardware lights and an override sun direction;
- binds `gPipeline.mAuxillaryRT.screen`;
- clears the auxiliary screen target;
- binds `gDeferredPBRAlphaProgram`;
- fixes shader constants for preview lighting;
- draws the sphere with `LLRenderPass::pushGLTFBatch(...)`;
- unbinds the deferred shader;
- runs the post-processing chain on the auxiliary screen target;
- swaps `mExposureMap` / `mLastExposure` around exposure generation;
- draws the final processed result into `mBoundTarget`;
- restores lights, reflection probe state, and `RenderLocalLightCount`;
- returns true.

## State To Preserve

Owner-local state:

- `mGLTFMaterial`;
- `mShouldRender`;
- `mBestLoad`;
- static preview sphere storage in `get_preview_sphere(...)`;
- `LLDrawInfo::mModelMatrix` pointing at `GLTFPreviewModel::mModelMatrix`.

Borrowed dynamic texture state:

- `mBoundTarget`;
- base `preRender(...)` viewport/camera setup;
- base `postRender(...)` framebuffer-to-texture copy and viewport/camera
  restore.

Borrowed pipeline/global state:

- `gPipeline.mRT`;
- `LLPipeline::RenderDepthOfField`;
- `LLPipeline::sRenderGlow`;
- `LLPipeline::RenderScreenSpaceReflections`;
- `LLPipeline::RenderFSAAType`;
- `gPipeline.mAuxillaryRT.screen`;
- `gPipeline.mSceneMap`;
- `gPipeline.mLuminanceMap`;
- `gPipeline.mExposureMap`;
- `gPipeline.mLastExposure`;
- `gPipeline.mPostPingMap`;
- `gPipeline.mTransformedSunDir`;
- `gPipeline.mScreenTriangleVB`;
- `gSavedSettings["RenderLocalLightCount"]`;
- reflection map default-probe state;
- `gGLLastMatrix`.

## Risk Notes

High-risk behaviors:

- changing `UIPreviewMaterial` fallback behavior;
- changing material load-level comparison semantics;
- changing the uncached preview creation policy;
- changing `ORDER_MIDDLE`;
- changing post-processing order;
- changing exposure map swap/restore ordering;
- changing `gPipeline.mRT` temporary routing;
- changing `mBoundTarget` usage for the final pass;
- changing shader constants or shader selection;
- changing cleanup order for lights, probes, or saved settings.

Medium-risk behaviors:

- owner-local helper extraction inside `render()`;
- naming the preview camera/sphere setup as a local helper;
- naming post-processing and final-pass sections as local helpers;
- documenting restore requirements.

## First Source Packet Candidate

If phase 5 source work continues here, the first packet should only split
`LLGLTFPreviewTexture::render()` into owner-local helpers.

Allowed helper boundaries:

- setup temporary preview pipeline state;
- setup preview camera and object transform;
- get/update preview sphere;
- draw alpha preview sphere;
- run preview post-processing chain;
- draw final processed result into `mBoundTarget`;
- restore preview state.

Not allowed in the first packet:

- changing material load policy;
- changing `LLTextureCtrl` or `LLFloaterTexturePicker`;
- changing `LLViewerDynamicTexture`;
- changing preview sphere geometry, UVs, or cached lifetime;
- changing shader constants;
- changing post-processing order;
- changing `RenderLocalLightCount` policy;
- changing exposure map swap timing.

## Verification Plan

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llgltfmaterialpreviewmgr.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Manual runtime smoke is optional for a pure helper extraction, but if requested
the relevant surface is a material texture picker with `UIPreviewMaterial`
enabled.

## Next Small Tasks

1. Apply the owner-local `render()` helper split only if the diff stays small.
2. Add a summary note after the source packet.
3. Do not touch `LLTextureCtrl` until the GLTF preview render owner is stable.
4. Map `LLViewerTexLayerSetBuffer` separately before avatar bake cleanup.
