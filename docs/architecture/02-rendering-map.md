# Rendering Map

Source: `docs/architecture/generated/source_inventory_top.md` and
`docs/architecture/generated/source_inventory.csv`.

This map is an analysis aid for phase 1. It is not a refactor plan. The
categories below are probable ownership buckets inferred from filenames,
inventory categories, line counts, and counters such as `gl_calls`, `gGL`,
`LLGL`, `LLPipeline`, `LLDrawPool`, `LLRenderTarget`, `LLImageGL`, and
`LLViewerTexture`.

Priority scale:

- P0: inspect first; central or high blast radius.
- P1: inspect early after P0; likely rendering contract surface.
- P2: inspect after boundaries are clearer.
- P3: keep listed, but do not spend early effort unless it blocks analysis.

Risk scale:

- Critical: touching this can break global rendering or platform startup.
- High: likely visible rendering regression or platform-specific breakage.
- Medium: localized rendering/UI/debug risk.
- Unknown: insufficient evidence from inventory alone.

## Frame Orchestration

Probable files:

- `indra/newview/pipeline.cpp`
- `indra/newview/llviewerdisplay.cpp`
- `indra/newview/llviewerwindow.cpp`
- `indra/newview/llappviewer.cpp`
- `indra/newview/llstartup.cpp`
- `indra/newview/llviewermenu.cpp`
- `indra/newview/RRInterface.cpp`

Risk: Critical.

Reason: these files sit around frame setup, viewer display, window integration,
pipeline ownership, menu/render mode entry points, and startup sequencing.
`pipeline.cpp` is the main hotspot in the inventory: 12046 lines, 83 direct
`gl*` references, 340 `gGL`, 158 `LLGL`, 564 `LLPipeline`, and 60
`LLRenderTarget` hits. `llviewerdisplay.cpp` and `llviewerwindow.cpp` also mix
frame control with viewer/window concerns.

Priority: P0.

## Legacy OpenGL Low-Level

Probable files:

- `indra/llrender/llgl.cpp`
- `indra/llrender/llglheaders.h`
- `indra/llrender/llgl.h`
- `indra/llrender/llglcommonfunc.cpp`
- `indra/llrender/llglstates.h`
- `indra/llrender/llrender.cpp`
- `indra/llrender/llrender2dutils.cpp`
- `indra/llrender/llvertexbuffer.cpp`
- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llimagegl.h`
- `indra/llrender/llgltexture.cpp`
- `indra/llrender/lltexture.cpp`

Risk: Critical.

Reason: these are the lowest-level OpenGL wrappers and state abstractions.
`llgl.cpp` alone contains 2213 direct `gl*` matches, and `llglheaders.h`
contains 762. This layer likely defines the current OpenGL contract for most
higher-level rendering code.

Priority: P0 for inventory only. Do not modify early.

## Render Targets

Probable files:

- `indra/llrender/llrendertarget.cpp`
- `indra/newview/pipeline.cpp`
- `indra/newview/llscenemonitor.cpp`
- `indra/newview/lldynamictexture.cpp`
- `indra/newview/llreflectionmapmanager.cpp`
- `indra/newview/llreflectionmap.cpp`
- `indra/newview/llheroprobemanager.cpp`
- `indra/newview/marefsr2upscaler.cpp`
- `indra/newview/marefsr2upscaler.h`
- `indra/newview/maretaaupscaler.cpp`
- `indra/newview/llscenemonitor.h`

Risk: High.

Reason: these files reference `LLRenderTarget` or allocate/use offscreen render
targets. `llrendertarget.cpp` is the low-level FBO surface; `pipeline.cpp`
coordinates many render targets; the reflection, scene monitor, hero probe, and
upscaler files appear to own specialized render targets.

Priority: P0 for `llrendertarget.cpp` and `pipeline.cpp`; P1 for specialized
users.

## Textures

Probable files:

- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llimagegl.h`
- `indra/newview/llviewertexture.cpp`
- `indra/newview/llviewertexture.h`
- `indra/newview/llviewertexturelist.cpp`
- `indra/newview/lltexturefetch.cpp`
- `indra/newview/lltexturecache.cpp`
- `indra/newview/lltexturectrl.cpp`
- `indra/newview/llterrainpaintmap.cpp`
- `indra/newview/gltf/asset.cpp`
- `indra/newview/gltf/llgltfloader.cpp`
- `indra/newview/llmeshrepository.cpp`
- `indra/newview/llsnapshotlivepreview.cpp`
- `indra/newview/llnetmap.cpp`
- `indra/llwindow/llwindowsdl.cpp`

Risk: High.

Reason: the inventory marks many of these as `assets.texture` or shows
`LLImageGL` / `LLViewerTexture` usage. Texture lifetime, upload, cache, fetch,
preview, and GLTF asset paths are likely cross-cutting and visible in many
viewer workflows.

Priority: P1.

## Shaders

Probable files:

- `indra/llrender/llglslshader.cpp`
- `indra/llrender/llglslshader.h`
- `indra/llrender/llshadermgr.cpp`
- `indra/llrender/llshadermgr.h`
- `indra/newview/llviewershadermgr.cpp`
- `indra/newview/llviewershadermgr.h`
- `indra/newview/app_settings/shaders/class1/deferred/pbrterrainF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/textureUtilV.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/CASF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/fxaaF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/fsr2/*.glsl`
- `indra/newview/app_settings/shaders/class1/interface/*.glsl`
- `indra/newview/app_settings/shaders/class1/avatar/*.glsl`
- `indra/newview/app_settings/shaders/class1/environment/*.glsl`
- `indra/newview/app_settings/shaders/class2/deferred/*.glsl`
- `indra/newview/app_settings/shaders/class3/deferred/*.glsl`

Risk: High.

Reason: shader manager files define compilation, binding, and uniform plumbing.
The shader tree is broad and the inventory classifies many shader files as
`viewer.misc`, `render.pipeline`, `assets.texture`, or `world.avatar`, which
means the current generated category is not enough to express real shader
ownership.

Priority: P0 for shader manager interfaces; P1 for shader families.

## Draw Pools

Probable files:

- `indra/newview/lldrawpool.cpp`
- `indra/newview/lldrawpool.h`
- `indra/newview/lldrawpoolterrain.cpp`
- `indra/newview/lldrawpoolmaterials.cpp`
- `indra/newview/lldrawpoolbump.cpp`
- `indra/newview/lldrawpooltree.cpp`
- `indra/newview/lldrawpoolwlsky.cpp`
- `indra/newview/lldrawpoolsimple.cpp`
- `indra/newview/lldrawpoolavatar.cpp`
- `indra/newview/lldrawpoolalpha.cpp`
- `indra/newview/lldrawpoolwater.cpp`
- `indra/newview/lldrawpoolsky.cpp`
- `indra/newview/lldrawpoolwaterexclusion.h`
- `indra/newview/lldrawpoolpbropaque.h`

Risk: High.

Reason: draw pools are direct rendering passes or pass-specific dispatch points.
The inventory shows `LLDrawPool` counts in terrain, materials, bump, sky,
simple, avatar, alpha, and water paths. These likely encode render ordering and
state assumptions.

Priority: P1.

## Avatar Rendering

Probable files:

- `indra/newview/llvoavatar.cpp`
- `indra/newview/llvoavatar.h`
- `indra/newview/llvoavatarself.cpp`
- `indra/newview/llvoavatarself.h`
- `indra/newview/lldrawpoolavatar.cpp`
- `indra/newview/llviewerjointmesh.cpp`
- `indra/newview/llviewerjoint.cpp`
- `indra/llappearance/lltexlayer.cpp`
- `indra/llappearance/llavatarappearance.cpp`
- `indra/llappearance/llavatarappearance.h`
- `indra/llappearance/llavatarjoint.cpp`
- `indra/llappearance/llavatarjointmesh.cpp`
- `indra/newview/llavatarrendernotifier.cpp`
- `indra/newview/llavatarrenderinfoaccountant.cpp`
- `indra/newview/app_settings/shaders/class1/avatar/*.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/avatar*.glsl`

Risk: High.

Reason: avatar code is large, stateful, and partly render-specific. `llvoavatar.cpp`
is 12711 lines and appears in the top OpenGL-related report. Avatar rendering
also crosses appearance, texture layers, draw pools, impostors, and shader
families.

Priority: P1.

## Terrain/World Rendering

Probable files:

- `indra/newview/lldrawpoolterrain.cpp`
- `indra/newview/llspatialpartition.cpp`
- `indra/newview/llvieweroctree.cpp`
- `indra/newview/llface.cpp`
- `indra/newview/llvovolume.cpp`
- `indra/newview/llviewerobject.cpp`
- `indra/newview/gltfscenemanager.cpp`
- `indra/newview/gltf/asset.cpp`
- `indra/newview/gltf/primitive.cpp`
- `indra/newview/llreflectionmapmanager.cpp`
- `indra/newview/llreflectionmap.cpp`
- `indra/newview/llheroprobemanager.cpp`
- `indra/newview/llviewerparceloverlay.cpp`
- `indra/newview/lldrawpoolwlsky.cpp`
- `indra/newview/lldrawpoolwater.cpp`
- `indra/newview/llterrainpaintmap.cpp`
- `indra/newview/app_settings/shaders/class1/deferred/pbrterrain*.glsl`
- `indra/newview/app_settings/shaders/class1/environment/waterF.glsl`

Risk: High.

Reason: this is the world visibility, geometry, terrain, water, GLTF, reflection,
and object rendering surface. It is tightly coupled to `pipeline.cpp`,
spatial partitioning, draw pools, and texture/material ownership.

Priority: P1, with `llspatialpartition.cpp`, `llface.cpp`, and `gltfscenemanager.cpp`
near the front.

## UI Rendering

Probable files:

- `indra/newview/llviewerwindow.cpp`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llfloaterregioninfo.cpp`
- `indra/newview/llpanelface.cpp`
- `indra/newview/llsnapshotlivepreview.cpp`
- `indra/newview/llnetmap.cpp`
- `indra/newview/llfloatermodelpreview.cpp`
- `indra/llui/llui.cpp`
- `indra/llui/llui.h`
- `indra/llui/lltextbase.cpp`
- `indra/llui/llmenugl.cpp`
- `indra/llui/lllocalcliprect.cpp`
- `indra/llui/llscrolllistctrl.cpp`
- `indra/llui/lltexteditor.cpp`
- `indra/llui/llview.cpp`
- `indra/llui/lllineeditor.cpp`
- `indra/llui/llbutton.cpp`
- `indra/llui/llfloater.cpp`
- `indra/llui/llpanel.cpp`
- `indra/llui/llstatbar.cpp`

Risk: Medium to High.

Reason: this category mixes actual UI widgets with preview surfaces and HUD-like
rendered UI. Several `llui` files are classified as `render.opengl_touching`;
several `newview` preview/floater files have `gGL`, `LLGL`, or draw-pool
signals. This is a likely boundary problem between UI and rendering.

Priority: P2 for pure UI; P1 for preview surfaces that render scene/model
content.

## Debug Rendering

Probable files:

- `indra/newview/llglsandbox.cpp`
- `indra/newview/llscenemonitor.cpp`
- `indra/newview/llfasttimerview.cpp`
- `indra/newview/lltextureview.cpp`
- `indra/newview/app_settings/shaders/class1/interface/debugV.glsl`
- `indra/newview/app_settings/shaders/class1/interface/debugF.glsl`
- `indra/newview/app_settings/shaders/class1/interface/normaldebugF.glsl`
- `indra/newview/app_settings/shaders/class1/interface/benchmarkV.glsl`
- `indra/newview/app_settings/shaders/class1/interface/benchmarkF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/postDeferredVisualizeBuffers.glsl`

Risk: Medium.

Reason: debug rendering tends to touch rendering state directly but is usually
less central to normal frame output. It is still risky because debug views can
share render targets, shaders, and global GL state with production passes.

Priority: P2.

## Unknown

Probable files:

- `indra/llmath/llvolume.cpp`
- `indra/llui/llfolderview.cpp`
- `indra/llui/llnotifications.cpp`
- `indra/llui/lltoolbar.cpp`
- `indra/llui/llview.h`
- `indra/llui/llmenugl.h`
- `indra/newview/SMAAAreaTex.h`
- `indra/newview/app_settings/shaders/class1/deferred/CASF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/fxaaF.glsl`
- `indra/newview/app_settings/shaders/class3/deferred/reflectionProbeF.glsl`
- `indra/newview/app_settings/shaders/class1/deferred/deferredUtil.glsl`

Risk: Unknown to Medium.

Reason: some files are marked `unknown` by the generated inventory despite
being plausibly related to geometry, UI, or rendering resources. Other shader
files are marked `viewer.misc` because the generator does not yet understand
shader ownership. These should not be ignored, but they need better
classification before architectural decisions.

Priority: P3, except shader files that are pulled into active render passes;
those should be promoted during shader-family inventory.

## Early Non-Modification Boundary

Per `AGENTS.md`, phase 1 should avoid modifying these source areas unless a
specific small task explicitly requests it:

- `indra/newview/`
- `indra/llrender/`
- `indra/llwindow/`
- `indra/llui/`

This map intentionally keeps work in documentation and analysis space.

