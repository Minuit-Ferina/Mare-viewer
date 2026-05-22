# Viewer Tex Layer Dynamic Texture Boundary

Date: 2026-05-22

## Scope

This note maps `LLViewerTexLayerSetBuffer`, the avatar bake/composite dynamic
texture path.

It should not be grouped with ordinary UI preview widgets even though it derives
from `LLViewerDynamicTexture`.

## Source Files

- `indra/newview/llviewertexlayer.h`
- `indra/newview/llviewertexlayer.cpp`
- `indra/llappearance/lltexlayer.cpp`

## Ownership Shape

`LLViewerTexLayerSetBuffer` derives from both:

- `LLTexLayerSetBuffer`
- `LLViewerDynamicTexture`

It is constructed as:

- `ORDER_LAST`
- 4 components
- unclamped dynamic texture

The constructor comment says `ORDER_LAST` is required because these buffers
must render after visual parameter hints are created.

## Dynamic Texture Adapter

The dynamic texture virtuals are adapters:

- `needsRender()` is implemented in `LLViewerTexLayerSetBuffer`.
- `preRender(bool)` forwards to `preRenderTexLayerSet()`.
- `render()` forwards to `renderTexLayerSet(mBoundTarget)`.
- `postRender(bool)` forwards to `postRenderTexLayerSet(success)`.

This means `LLViewerDynamicTexture::updateAllInstances()` controls target
selection, viewport/camera save/restore, and bound-target assignment, while
the texture-layer code controls the composite render contract.

## Render Target

Because this user is `ORDER_LAST`, `updateAllInstances()` renders it into:

- `gPipeline.mBakeMap`

not the general preview target.

The selected target is passed indirectly through `mBoundTarget` into:

- `LLTexLayerSetBuffer::renderTexLayerSet(LLRenderTarget* bound_target)`

## Render Flow

High-level flow:

1. `LLViewerDynamicTexture::updateAllInstances()` selects `gPipeline.mBakeMap`.
2. It calls `needsRender()`.
3. It assigns `mBoundTarget`.
4. It calls `preRender()`, which forwards to `preRenderTexLayerSet()`.
5. `LLViewerTexLayerSetBuffer::preRenderTexLayerSet()` calls the base
   `LLTexLayerSetBuffer::preRenderTexLayerSet()`, then calls
   `LLViewerDynamicTexture::preRender(false)`.
6. `render()` calls `renderTexLayerSet(mBoundTarget)`.
7. The base texture-layer render path binds `gAlphaMaskProgram`, sets alpha
   minimum, renders `mTexLayerSet`, flushes, calls `midRenderTexLayerSet()`,
   then resets color mask and alpha blending.
8. `postRender()` forwards to `postRenderTexLayerSet(success)`.
9. `LLViewerTexLayerSetBuffer::postRenderTexLayerSet()` calls the base
   post-render hook, then `LLViewerDynamicTexture::postRender(success)`.

## Readiness Rules

`needsRender()` requires:

- the texture layer set belongs to `gAgentAvatarp`;
- the agent avatar is valid;
- `mNeedsUpdate` is true;
- `isReadyToUpdate()` is true;
- appearance is not currently animating;
- skirt bake is skipped when the avatar is not wearing a skirt;
- local texture data is available.

`isReadyToUpdate()` allows work when:

- local texture data is final;
- no low-resolution update has happened yet;
- or a timeout has elapsed and lower-LOD data is available.

## State Updates

`midRenderTexLayerSet(bool success)`:

- calls `doUpdate()` when an update is still needed and ready;
- marks the GL texture created regardless of `success`, preserving the current
  old behavior noted by the source comment.

`doUpdate()`:

- clears `mNeedsUpdate` when final local texture data is available;
- otherwise increments `mNumLowresUpdates`.

## Risk

Risk: high.

Why:

- This path affects avatar baked texture compositing.
- It depends on dynamic texture ordering relative to visual parameter hints.
- It borrows `gPipeline.mBakeMap`.
- It bridges `newview` avatar state with `llappearance` layer compositing.
- It deliberately preserves old behavior around `setGLTextureCreated()` and
  render success.

## Guardrail

Do not combine this with ordinary preview UI cleanup.

Any future task here should state whether it changes:

- `ORDER_LAST` ordering;
- `gPipeline.mBakeMap` usage;
- readiness checks;
- low-resolution update timing;
- alpha mask shader state;
- `setGLTextureCreated()` behavior;
- `mBoundTarget` handoff.
