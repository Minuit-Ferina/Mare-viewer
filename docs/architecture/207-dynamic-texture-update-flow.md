# Dynamic Texture Update Flow

Date: 2026-05-22

## Scope

This note maps `LLViewerDynamicTexture::updateAllInstances()` before any source
changes to the dynamic texture driver.

This path is UI-adjacent but not ordinary UI drawing. It renders preview and
avatar-bake content into render targets, then copies that content into viewer
textures.

## Callers

Known direct callers:

- `display_startup()` in `indra/newview/llviewerdisplay.cpp`
  - Called after the first two startup frames.
  - Comment says this is required for HTML update in the login screen.
- `display()` in `indra/newview/llviewerdisplay.cpp`
  - Called when
    `LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES` is enabled.
  - If any dynamic texture rendered, it restores the color mask and clears the
    depth buffer before continuing scene rendering.

## Static State

`LLViewerDynamicTexture` owns:

- `sInstances[ORDER_COUNT]`
- `sNumRenders`

Instances register themselves in one order bucket during construction and
remove themselves from all buckets during destruction.

Order buckets:

- `ORDER_FIRST`
- `ORDER_MIDDLE`
- `ORDER_LAST`
- `ORDER_RESET`

Current comments in `lldynamictexture.cpp` describe `ORDER_FIRST` as unused,
`ORDER_MIDDLE` as UI preview work, `ORDER_LAST` as baked skin preview, and
`ORDER_RESET` as appearance reset work that does not render.

## Render Targets

`updateAllInstances()` uses two pipeline-owned render targets:

- `gPipeline.mAuxillaryRT.deferredScreen`
  - Used for orders before `ORDER_LAST`.
  - Size must cover `LLPipeline::MAX_PREVIEW_WIDTH`.
- `gPipeline.mBakeMap`
  - Used for `ORDER_LAST` through `ORDER_RESET`.
  - Size must cover `LLAvatarAppearanceDefines::SCRATCH_TEX_WIDTH` and
    `SCRATCH_TEX_HEIGHT`.

Both targets must be complete before rendering. If either is incomplete, the
function asserts and returns `false`.

## Flow

Top-level flow:

1. Reset `sNumRenders`.
2. Return `true` immediately if GL is disabled.
3. Validate preview and bake render targets.
4. Bind and clear the preview target.
5. Unbind the active shader and vertex buffer.
6. Render all instances in `ORDER_FIRST` and `ORDER_MIDDLE`.
7. Flush the preview target.
8. Bind and clear the bake target.
9. Render all instances in `ORDER_LAST` and `ORDER_RESET`.
10. Flush the bake target.
11. Flush `gGL`.
12. Return whether any dynamic texture actually rendered.

Per-instance flow inside the local `update_func`:

1. Skip if `needsRender()` returns false.
2. Assert the dynamic texture fits in the selected render target.
3. Clear depth.
4. Set the immediate color to white.
5. Store the selected render target with `setBoundTarget()`.
6. Call `preRender()`.
7. Call the virtual `render()`.
8. If rendering succeeded, increment `sNumRenders`.
9. Flush `gGL`.
10. Unbind the vertex buffer.
11. Clear the dynamic texture's bound-target pointer.
12. Call `postRender(result)`.

## Default Pre/Post Behavior

Default `preRender()`:

- resets `mOrigin` to the lower-left corner;
- unbinds texture unit 0;
- snapshots the current viewer camera;
- sets the dynamic texture viewport;
- optionally clears depth.

Default `postRender(success)`:

- ensures the dynamic texture has a valid GL texture;
- copies from the framebuffer into the dynamic texture with
  `setSubImageFromFrameBuffer()`;
- restores the 2D viewport through `gViewerWindow->setup2DViewport()`;
- restores the viewer camera origin, axes, aspect, view, and near plane.

## Known Dynamic Texture Users

Known subclasses or direct users:

- `LLModelPreview`
- `LLImagePreviewAvatar`
- `LLImagePreviewSculpted`
- `LLViewerTexLayerSetBuffer`
- `LLVisualParamHint`
- `LLVisualParamReset`
- `LLGLTFPreviewTexture`
- `LLPreviewAnimation`

Not all users rely only on the default pre/post behavior. Examples:

- `LLVisualParamHint::preRender()` mutates avatar visual parameters before
  delegating to `LLViewerDynamicTexture::preRender()`.
- `LLViewerTexLayerSetBuffer` wraps dynamic texture pre/post in texture-layer
  set pre/post hooks.
- `LLGLTFPreviewTexture::preRender()` and `postRender()` guard work on
  `mShouldRender` and delegate to the base implementation.

## Ownership Boundaries

`LLViewerDynamicTexture` owns:

- instance ordering;
- target selection between preview and bake targets;
- viewport setup for dynamic texture dimensions;
- camera save/restore for the default path;
- framebuffer-to-texture copy for the default `postRender()`.

The subclasses own:

- whether they need rendering;
- actual preview scene/widget/avatar/material rendering;
- any local avatar, material, texture, or shader state they mutate.

`LLPipeline` owns the render targets. `LLViewerDynamicTexture` borrows them; it
does not allocate or resize them.

## Do Not Change First

Do not change these behaviors without a focused task and visual validation:

- order split between preview target and bake target;
- `preRender()` before virtual `render()`;
- `postRender()` after `setBoundTarget(nullptr)`;
- camera save/restore;
- `gViewerWindow->setup2DViewport()` restoration;
- shader and vertex-buffer unbinds before preview rendering;
- depth clear behavior before each dynamic texture render.

## Small Next Tasks

- Add a per-subclass table with order bucket, render target, and user-visible
  feature.
- Split avatar bake dynamic textures from UI preview dynamic textures.
- Document which dynamic texture users can run on the login screen.
- Only then consider a containment task around viewport/camera restoration.
