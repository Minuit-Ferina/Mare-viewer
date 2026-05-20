# Render Target Lifecycle

This document maps current `LLRenderTarget` ownership and lifecycle behavior.
It is a phase 1 analysis artifact only. It does not propose source edits.

## Files Inspected

- `indra/newview/pipeline.h`
- `indra/newview/pipeline.cpp`
- `indra/llrender/llrendertarget.h`
- `indra/llrender/llrendertarget.cpp`
- `indra/newview/lldynamictexture.cpp`
- `indra/newview/llgltfmaterialpreviewmgr.cpp`
- `indra/newview/llreflectionmapmanager.cpp`
- `indra/newview/llscenemonitor.cpp`
- `indra/newview/llviewerdisplay.cpp`

## Ownership Model

`LLRenderTarget` is the low-level FBO/texture owner. It stores:

- color texture IDs in `mTex`
- color formats in `mInternalFormat`
- one FBO ID in `mFBO`
- optional depth texture ID in `mDepth`
- shared-depth state in `mUseDepth`
- render target stack links through `mPreviousRT`

It also owns global render-target binding state:

- `LLRenderTarget::sBoundTarget`
- `LLRenderTarget::sCurFBO`
- `LLRenderTarget::sCurResX`
- `LLRenderTarget::sCurResY`
- `LLRenderTarget::sBytesAllocated`

`LLPipeline` is the main high-level owner. It groups core scene targets inside
`RenderTargetPack`:

| pack member | purpose |
|---|---|
| `screen` | main color target used as the scene color source. |
| `deferredScreen` | deferred G-buffer target with multiple attachments. |
| `deferredLight` | lighting/post intermediate target. |
| `shadow[4]` | sun shadow maps for the active target pack. |

`LLPipeline` owns three packs:

| pack | likely role | risk |
|---|---|---|
| `mMainRT` | normal world rendering at viewer resolution. | Critical |
| `mAuxillaryRT` | reflection probes and dynamic texture preview/bake paths. | High |
| `mHeroProbeRT` | hero probe rendering at probe-face size. | High |

The active pack is selected by `LLPipeline::mRT`. This pointer normally points
to `mMainRT`, but `allocateScreenBufferInternal()` temporarily redirects it to
`mAuxillaryRT` and `mHeroProbeRT` while allocating probe resources.

## Specialized Pipeline Targets

`LLPipeline` also owns these standalone render targets:

| target | probable lifecycle/purpose | risk |
|---|---|---|
| `mSpotShadow[2]` | spot shadow maps when shadow detail is high enough. | High |
| `mPbrBrdfLut` | generated BRDF lookup target. | Medium |
| `mWaterExclusionMask` | water plane exclusion mask with its own depth buffer. | High |
| `mSceneMap` | copy of scene color/depth before gamma correction, used by SSR. | High |
| `mLuminanceMap` | mipmapped luminance sampling target. | Medium |
| `mExposureMap` | 1x1 exposure result. | Medium |
| `mLastExposure` | previous-frame exposure history. | Medium |
| `mPostPingMap` | post-processing ping target. | High |
| `mPostPongMap` | post-processing pong target. | High |
| `mFXAAMap` | FXAA/SMAA helper target. | High |
| `mSMAABlendBuffer` | SMAA blend helper target. | High |
| `mVelocityBuffer` | Mare motion-vector buffer, allocated only when upscaler is enabled. | High |
| `mDisplayScreen` | Mare FSR2 display-resolution output target. | High |
| `mUIScreen` | optional UI buffer target. | Medium |
| `mDownResMap` | GPU downscale scratch target. | Medium |
| `mBakeMap` | baked avatar/appearance scratch target. | Medium |
| `mWaterDis` | water distortion/refraction scratch target. | High |
| `mGlow[3]` | glow extraction and blur ping-pong targets. | High |

External owners also allocate render targets:

- `LLVOAvatar::mImpostor` for avatar impostor rendering.
- `LLSceneMonitor::mFrames[]` and `mDiff` for scene-monitor capture/diff.
- Reflection map code uses `gPipeline.mAuxillaryRT.screen` and a local
  `mRenderTarget` for probe downsampling.
- Dynamic texture code uses `gPipeline.mAuxillaryRT.deferredScreen` and
  `gPipeline.mBakeMap`.
- GLTF material preview uses `gPipeline.mAuxillaryRT.screen`.

## Allocation Flow

`LLPipeline::init()` sets `mRT = &mMainRT`.

`LLPipeline::createGLBuffers()`:

1. Allocates `mGlow[0..2]`.
2. Calls `allocateScreenBuffer()` using the current world view size.
3. Creates non-render-target GL textures such as noise maps and SMAA lookup
   textures.
4. Calls `createLUTBuffers()` for BRDF, exposure, luminance, and exposure
   history targets.

`LLPipeline::resizeScreenTexture()`:

1. Checks whether shaders are loaded.
2. Reads the raw world view size.
3. If size changed or resize was requested, releases screen buffers and shadow
   targets.
4. Calls `allocateScreenBuffer()` again.

`LLPipeline::allocateScreenBuffer()` delegates to
`doAllocateScreenBuffer()`. On failure, the code releases partial state and
tries lower resolutions by halving `resY`, then `resX`.

`LLPipeline::allocateScreenBufferInternal()` is the main allocation hotspot:

1. If `mRT == &mMainRT`, it temporarily enables `gCubeSnapshot`, initializes
   reflection/hero probes, and recursively allocates `mAuxillaryRT` and
   optionally `mHeroProbeRT`.
2. It restores `mRT` to `mMainRT` and disables `gCubeSnapshot`.
3. It stores full display dimensions in `mRT->width` and `mRT->height`.
4. It applies `RenderResolutionDivisor`.
5. It conditionally applies Mare FSR2 render-resolution preset scaling.
6. It allocates `mRT->deferredScreen` with depth.
7. It calls `addDeferredAttachments()` to add G-buffer attachments.
8. It allocates `mRT->screen`.
9. It shares `deferredScreen` depth into `screen`.
10. It allocates or releases `mRT->deferredLight`.
11. It calls `allocateShadowBuffer()`.
12. If not in cube-snapshot allocation, it allocates UI, anti-aliasing, water,
    SSR, post, water-exclusion, downscale, bake, velocity, display, and upscaler
    resources.

## Low-Level `LLRenderTarget` Lifecycle

`LLRenderTarget::allocate()`:

1. Asserts the target is not bound in the render-target stack.
2. Clamps requested dimensions to `gGLManager.mGLMaxTextureSize`.
3. Calls `release()` before rebuilding resources.
4. Stores resolution, usage, depth, and mip generation settings.
5. Optionally calls `allocateDepth()`.
6. Generates an FBO.
7. Attaches depth if present.
8. Calls `addColorAttachment()` for the first color target.

`LLRenderTarget::addColorAttachment()`:

1. Asserts the target is not bound.
2. Rejects more than four color attachments.
3. Generates a texture through `LLImageGL`.
4. Allocates texture storage with `LLImageGL::setManualImage()`.
5. Sets filtering and address mode.
6. Attaches the texture to the FBO when an FBO exists.
7. Stores texture ID and internal format.
8. In debug GL mode, binds and flushes the target to validate it.

`LLRenderTarget::shareDepthBuffer()` attaches one target's `mDepth` texture to
another target's FBO and marks the receiver as using shared depth. In the main
pipeline this is used for `mRT->deferredScreen.shareDepthBuffer(mRT->screen)`.

`LLRenderTarget::bindTarget()`:

1. Asserts the FBO exists and the target is not already bound in the stack.
2. Binds the FBO.
3. Configures draw/read buffers based on color attachment count.
4. Checks framebuffer status.
5. Sets viewport to target dimensions.
6. Pushes the previous bound render target through `mPreviousRT` and
   `sBoundTarget`.

`LLRenderTarget::flush()`:

1. Calls `gGL.flush()`.
2. Asserts the target is current.
3. Generates mipmaps for auto-mipmap targets.
4. If there was a previous target, rebinds it.
5. Otherwise binds framebuffer 0, restores the global viewport from
   `gGLViewport`, and restores back-buffer read/draw buffers.

`LLRenderTarget::release()`:

1. Asserts the target is not bound in the stack.
2. Deletes owned depth texture if present.
3. Detaches shared depth if needed.
4. Detaches and deletes extra color attachments.
5. Deletes the FBO.
6. Deletes the primary color texture.
7. Clears texture/format vectors and zeroes dimensions.

## Main Runtime Uses

| stage | targets involved | notes |
|---|---|---|
| deferred geometry | `mRT->deferredScreen` | Draw pools write G-buffer attachments. |
| deferred lighting | `mRT->deferredLight`, `mRT->screen`, `mVelocityBuffer` | Lighting, SSAO/shadows, blur passes, and Mare velocity passes bind/flush multiple targets. |
| shadows | `mRT->shadow[0..3]`, `mSpotShadow[0..1]` | Allocated by `allocateShadowBuffer()` and accessed through `getSunShadowTarget()` / `getSpotShadowTarget()`. |
| post processing | `mPostPingMap`, `mPostPongMap`, `mRT->deferredLight`, `mDisplayScreen` | `renderFinalize()` ping-pongs through tonemap, CAS, glow, DoF, FXAA/SMAA, debug visualization, and RLV effects. |
| SSR/luminance/exposure | `mSceneMap`, `mLuminanceMap`, `mExposureMap`, `mLastExposure` | Uses scene color and G-buffer depth/normal attachments. |
| water | `mWaterDis`, `mWaterExclusionMask` | Water rendering writes separate distortion and exclusion targets. |
| UI/dynamic previews | `mAuxillaryRT.deferredScreen`, `mAuxillaryRT.screen`, `mBakeMap` | UI-adjacent code renders previews and baked texture scratch data into pipeline-owned targets. |
| avatar impostors | `LLVOAvatar::mImpostor` | Avatar code allocates/resizes/binds its own target and adds deferred attachments when needed. |
| scene monitor | `LLSceneMonitor::mFrames[]`, `LLSceneMonitor::mDiff` | Captures/copies framebuffer content and compares frames, including direct framebuffer binding. |

## High-Risk Boundaries

- `mRT` is mutable global-ish pipeline state. Recursive allocation temporarily
  swaps it away from `mMainRT`.
- `gCubeSnapshot` changes allocation and rendering behavior during auxiliary
  target allocation.
- `mRT->deferredScreen` owns the depth buffer shared into `mRT->screen`.
- `LLRenderTarget::bindTarget()` / `flush()` implements an implicit target stack.
  Missing or extra flushes can corrupt later render passes.
- `LLRenderTarget::release()` asserts the target is not bound. Releasing during
  nested rendering would be unsafe.
- `LLRenderTarget::resize()` has an explicit header warning: do not use it for
  screen-space buffers or important scratch space if allocation failure would be
  damaging.
- Post-processing dimensions can differ from render dimensions when Mare FSR2 is
  active: `mDisplayScreen` and post targets can use display resolution while
  scene/G-buffer targets use render resolution.
- UI preview and dynamic texture paths borrow pipeline-owned targets rather than
  owning a separate UI render surface.

## Areas Not To Modify First

- `LLPipeline::allocateScreenBufferInternal()` allocation order.
- `LLPipeline::releaseScreenBuffers()` and shadow release behavior.
- `LLRenderTarget::bindTarget()` / `flush()` stack semantics.
- `LLRenderTarget::release()` ownership and deletion order.
- The depth-sharing relationship between `deferredScreen` and `screen`.
- The `mRT` pointer model for main, auxiliary, and hero probe target packs.
- The Mare upscaler target sizing logic until the FSR2/Darwin build boundary is
  fully documented.

## Small Follow-Up Tasks

- Draw a narrow sequence map for `renderFinalize()` post target ping-pong.
- Add a UI render-boundary document for preview, map, HUD, and `llui` users.
- Add a platform OpenGL note for SDL, Windows, Darwin, and Mesa/headless context
  paths.
- Consider improving the inventory generator to report raw `gl*` references and
  likely call expressions separately.
