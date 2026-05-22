# Dynamic Texture User Table

Date: 2026-05-22

## Scope

This note lists known `LLViewerDynamicTexture` users by order bucket, render
target group, feature surface, and risk.

It builds on `docs/architecture/207-dynamic-texture-update-flow.md`.

## User Table

| user | source | order | target group | visible feature | risk |
|---|---|---|---|---|---|
| `LLModelPreview` | `indra/newview/llmodelpreview.cpp` | `ORDER_MIDDLE` | preview target | model upload preview | high |
| `LLImagePreviewAvatar` | `indra/newview/llfloaterimagepreview.cpp` | `ORDER_MIDDLE` | preview target | image upload avatar preview | medium-high |
| `LLImagePreviewSculpted` | `indra/newview/llfloaterimagepreview.cpp` | `ORDER_MIDDLE` | preview target | sculpted image preview | medium-high |
| `LLPreviewAnimation` | `indra/newview/llfloaterbvhpreview.cpp` | `ORDER_MIDDLE` | preview target | BVH animation upload preview | medium-high |
| `LLVisualParamHint` | `indra/newview/lltoolmorph.cpp` | `ORDER_MIDDLE` | preview target | appearance editor visual parameter hint | high |
| `LLGLTFPreviewTexture` | `indra/newview/llgltfmaterialpreviewmgr.cpp` | `ORDER_MIDDLE` | preview target | GLTF material preview in texture/material UI | high |
| `LLViewerTexLayerSetBuffer` | `indra/newview/llviewertexlayer.cpp` | `ORDER_LAST` | bake target | avatar baked texture compositing | high |
| `LLVisualParamReset` | `indra/newview/lltoolmorph.cpp` | `ORDER_RESET` | bake target group | appearance parameter reset path; comment says it does not render | medium |

Target groups:

- preview target: `gPipeline.mAuxillaryRT.deferredScreen`
- bake target: `gPipeline.mBakeMap`

## Order Notes

`ORDER_MIDDLE` users render before avatar bake users. This matters because
`LLViewerTexLayerSetBuffer` is constructed with a comment saying `ORDER_LAST`
must render after hints are created.

`ORDER_RESET` is processed in the bake-target group because
`updateAllInstances()` loops from `ORDER_LAST` through `ORDER_COUNT` after
binding `gPipeline.mBakeMap`.

## Login-Screen Uncertainty

`display_startup()` calls `LLViewerDynamicTexture::updateAllInstances()` after
the first two startup frames, with a comment that this is required for HTML
update in the login screen.

The current source search does not prove which subclass instances are present
on the login screen. Most known subclasses are tied to upload, appearance,
texture/material preview, avatar bake, or animation preview workflows.

Before changing startup behavior, inspect runtime instance creation rather than
assuming the login-screen caller is unused.

## Risk Groups

High-risk users:

- `LLModelPreview`
- `LLVisualParamHint`
- `LLGLTFPreviewTexture`
- `LLViewerTexLayerSetBuffer`

Why:

- they mutate avatar, material, shader, or pipeline target state;
- they rely on camera/viewport restoration from the base class;
- regressions may be visible only in specific upload, appearance, or material
  workflows.

Medium-high users:

- `LLImagePreviewAvatar`
- `LLImagePreviewSculpted`
- `LLPreviewAnimation`

Why:

- they are UI preview paths but still render scene-like content into dynamic
  textures.

Medium user:

- `LLVisualParamReset`

Why:

- the current code comments say it does not render, but it still participates
  in the bake-target order range.

## Small Next Tasks

- Map each user's `needsRender()`, `preRender()`, `render()`, and
  `postRender()` overrides.
- Add a login-screen runtime observation only if a manual test is requested.
- Keep `LLViewerTexLayerSetBuffer` separate from ordinary UI preview cleanup.
