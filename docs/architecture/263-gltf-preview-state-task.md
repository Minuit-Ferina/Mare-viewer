# GLTF Preview State Task

Date: 2026-05-23

Branch: `phase5`

## Scope

This task follows `docs/architecture/262-gltf-preview-render-split-summary.md`.

The source change may only touch the `LLGLTFPreviewTexture` owner in
`indra/newview/llgltfmaterialpreviewmgr.cpp`.

## Goal

Group the temporary GLTF preview render state into one owner-local RAII object.

The goal is not to change rendering. The goal is to make the preview state
lifetime explicit before any future UI/render boundary cleanup.

## State To Group

Current state setup in `LLGLTFPreviewTexture::render()` includes:

- depth test disabled through `LLGLDepthTest`;
- stencil disabled through `LLGLDisable`;
- scissor disabled through `LLGLDisable`;
- `LLPipeline::RenderDepthOfField` temporarily false;
- `LLPipeline::sRenderGlow` temporarily false;
- `LLPipeline::RenderScreenSpaceReflections` temporarily false;
- `LLPipeline::RenderFSAAType` temporarily zero;
- `gPipeline.mRT` temporarily set to `gPipeline.mAuxillaryRT`;
- `RenderLocalLightCount` temporarily set to zero;
- reflection map default-probe state forced for preview rendering.

## Ordering To Preserve

The packet must preserve this order:

1. `mShouldRender` guard.
2. Clear transparent color and depth.
3. Set temporary GL and pipeline preview state.
4. Force local light count to zero.
5. Force default reflection probe state.
6. Render preview camera, sphere, post-processing, and final pass.
7. Restore hardware lights, reflection probe state, and
   `RenderLocalLightCount`.
8. Let temporary GL and pipeline RAII objects restore their previous values.

## Not Allowed

Do not change:

- material load policy;
- UI entry points;
- shader selection;
- shader constants;
- preview sphere geometry or UVs;
- post-processing order;
- render target selection;
- `mBoundTarget` usage;
- dynamic texture lifecycle.

## Verification

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
