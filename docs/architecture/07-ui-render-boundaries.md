# UI Render Boundaries

This document maps UI-facing code that also touches rendering primitives. It is
a phase 1 analysis artifact only. It does not propose source edits.

Source inputs:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/03-opengl-debt.md`
- `docs/architecture/04-gl-callsite-inventory.md`
- targeted reads of the files listed below

## Files Inspected

- `indra/newview/lldynamictexture.cpp`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llgltfmaterialpreviewmgr.cpp`
- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llsnapshotlivepreview.cpp`
- `indra/newview/lltexturectrl.cpp`
- `indra/newview/llfloatermodelpreview.cpp`
- `indra/newview/llnetmap.cpp`
- `indra/newview/llworldmapview.cpp`
- `indra/llui/lllocalcliprect.cpp`
- `indra/llui/llview.cpp`
- `indra/llui/lltextbase.cpp`
- `indra/llui/llmenugl.cpp`
- `indra/llui/llscrolllistctrl.cpp`
- `indra/llui/lltexteditor.cpp`
- `indra/llui/llbutton.cpp`
- `indra/llui/llfloater.cpp`
- `indra/llui/llui.cpp`
- `indra/llui/llui.h`

## Inventory Snapshot

| file | category in generated inventory | gl_calls | gGL | LLGL | LLRenderTarget | boundary type |
|---|---|---:|---:|---:|---:|---|
| `indra/newview/llmodelpreview.cpp` | `render.opengl_touching` | 17 | 58 | 13 | 0 | model upload preview renders scene-like content in UI |
| `indra/newview/llsnapshotlivepreview.cpp` | `assets.texture` | 3 | 64 | 0 | 0 | snapshot preview draws viewer textures and overlay effects |
| `indra/newview/llgltfmaterialpreviewmgr.cpp` | `render.pipeline` | 3 | 2 | 45 | 1 | material preview renders through dynamic texture and deferred shader paths |
| `indra/newview/lldynamictexture.cpp` | `render.pipeline` | 3 | 4 | 5 | 3 | shared dynamic texture render target driver |
| `indra/newview/llnetmap.cpp` | `assets.texture` | 1 | 92 | 0 | 0 | minimap UI draws map textures and vector overlays |
| `indra/newview/llfloaterimagepreview.cpp` | `render.draw_pool` | 1 | 72 | 7 | 0 | upload/image preview draws textures and avatar/sculpt preview content |
| `indra/llui/lllocalcliprect.cpp` | `render.opengl_touching` | 1 | 1 | 0 | 0 | core UI scissor boundary |
| `indra/newview/llworldmapview.cpp` | `render.opengl_touching` | 0 | 88 | 2 | 0 | world map UI draws fetched map textures and vector overlays |
| `indra/newview/lltexturectrl.cpp` | `assets.texture` | 0 | 0 | 5 | 0 | texture picker owns preview selection and material preview access |
| `indra/newview/llfloatermodelpreview.cpp` | `ui` | 0 | 17 | 0 | 0 | model upload floater drives `LLModelPreview` |
| `indra/llui/lltextbase.cpp` | `render.opengl_touching` | 0 | 5 | 0 | 0 | text rendering, selection, cursor, inline images, clipping |
| `indra/llui/llfloater.cpp` | `ui` | 0 | 23 | 1 | 0 | floater frame rendering and context cone |
| `indra/llui/llview.cpp` | `render.opengl_touching` | 0 | 14 | 0 | 0 | UI traversal, matrix state, debug rect drawing |
| `indra/llui/llbutton.cpp` | `render.opengl_touching` | 0 | 4 | 0 | 0 | image/font button rendering and blend mode changes |

Inventory caveat: `indra/llui/llui.cpp` and `indra/llui/llui.h` have raw
`gl*` matches for `glPointToScreen` and `glRectToScreen`. These are coordinate
helper names, not OpenGL API calls.

## Boundary Groups

### Dynamic Preview Driver

Primary file: `indra/newview/lldynamictexture.cpp`

`LLViewerDynamicTexture` is the common driver for several UI preview surfaces.
It owns a list of dynamic texture instances by render order, sets the viewport,
and updates instances through shared render targets:

- `gPipeline.mAuxillaryRT.deferredScreen`
- `gPipeline.mBakeMap`

The update path binds a render target, calls `preRender()`, calls the preview's
`render()`, flushes `gGL`, then calls `postRender()`.

Risk: high. This file is small, but it controls when UI-adjacent previews borrow
pipeline-owned render targets. Any containment work here can affect model
upload preview, material preview, baked texture preview, and other dynamic
texture users at once.

Priority: P0 for analysis, no source edits until every dynamic texture subclass
used by viewer UI is listed.

### Model And Upload Preview

Primary files:

- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.cpp`
- `indra/newview/llfloaterimagepreview.cpp`

`LLModelPreview` derives from `LLViewerDynamicTexture` and renders model upload
content inside the UI. The render path uses `gGL` matrix state, vertex buffers,
preview materials, physics hull drawing, debug outlines, and direct OpenGL state
calls such as `glLineWidth`, `glPolygonMode`, `glClear`, and `glPointSize`.

`LLFloaterModelPreview` is the UI owner around that preview. It should be
treated as part of the same boundary even though the heaviest rendering code is
in `LLModelPreview`.

`LLFloaterImagePreview` draws uploaded texture previews and also contains
preview paths for avatar and sculpted content. It uses `gGL` texture binding,
manual triangle drawing, UI matrices, `LLGLSUIDefault`, and a direct depth clear.

Risk: high. These files mix UI controls, upload workflow state, preview camera
state, texture binding, vertex buffers, and direct GL state.

Priority: P1 for analysis after the dynamic preview driver is fully mapped.

### GLTF Material Preview

Primary files:

- `indra/newview/llgltfmaterialpreviewmgr.cpp`
- `indra/newview/lltexturectrl.cpp`

`LLGLTFPreviewTexture` derives from `LLViewerDynamicTexture`. It fetches GLTF
material textures, boosts preview loads, clears the preview target, temporarily
switches `gPipeline.mRT` to `gPipeline.mAuxillaryRT`, binds
`gPipeline.mAuxillaryRT.screen`, and renders through `gDeferredPBRAlphaProgram`.

`LLTextureCtrl` is the UI entry point that asks the GLTF material preview manager
for preview content when a texture control is displaying material state.

Risk: high. This is a UI control path that reaches material fetch state,
deferred shader state, pipeline render targets, and preview texture lifetime.

Priority: P1 for analysis. Do not change this before shader and render target
lifecycle assumptions are stable.

### Snapshot Preview

Primary file: `indra/newview/llsnapshotlivepreview.cpp`

`LLSnapshotLivePreview` draws the current snapshot image and animated UI effects
with `gGL` texture binding and immediate triangle drawing. The preview rectangle
uses direct line-width state through `glGetFloatv` and `glLineWidth`.

The snapshot generation path also passes UI/HUD/balance/post-processing flags
into viewer snapshot capture.

Risk: medium-high. The visible drawing is UI-like, but snapshot generation has
runtime render-policy inputs. It is a boundary between UI preview, capture
settings, and viewer render output.

Priority: P1 for analysis, P2 for containment after preview target ownership is
clear.

### Map And Minimap Widgets

Primary files:

- `indra/newview/llnetmap.cpp`
- `indra/newview/llworldmapview.cpp`

`LLNetMap::draw()` is a UI control draw method that resets UI/model matrices,
binds region, object, and parcel textures, emits triangles through `gGL`, draws
vector overlays, and calls a direct `glMatrixMode(GL_MODELVIEW)` in the ring
helper path.

`LLWorldMapView::draw()` and helpers draw fetched world-map textures, overlays,
agent dots, frustums, tracking markers, text labels, and UI images. It does not
show likely direct OpenGL calls in the refined inventory, but it is one of the
top `gGL` users outside low-level render code.

Risk: medium-high. These widgets are UI controls, but they also interpret world
and map state and emit geometry directly. Transform assumptions may be fragile.

Priority: P1 for analysis. Treat minimap and world map as separate UI rendering
subsystems before proposing a shared abstraction.

### Core `llui` Rendering Primitives

Primary files:

- `indra/llui/lllocalcliprect.cpp`
- `indra/llui/llview.cpp`
- `indra/llui/lltextbase.cpp`
- `indra/llui/llmenugl.cpp`
- `indra/llui/llscrolllistctrl.cpp`
- `indra/llui/lltexteditor.cpp`
- `indra/llui/llbutton.cpp`
- `indra/llui/llfloater.cpp`
- `indra/llui/llui.cpp`
- `indra/llui/llui.h`

`LLScreenClipRect::updateScissorRegion()` flushes `gGL` and calls
`glScissor()`. This is the clearest direct OpenGL dependency in core UI.

`LLView` owns traversal and child drawing. It also sets matrix state and draws
debug rectangles through `gGL`.

`LLTextBase`, `LLTextEditor`, and scroll/list controls combine UI clipping,
font rendering, selection/cursor drawing, inline images, and texture unbinds.

`LLButton` and `LLFloater` draw UI images and text. `LLButton` can alter scene
blend type for glow handling. `LLFloater::drawConeToOwner()` emits a triangle
strip through `gGL` and enables culling through `LLGLEnable`.

Risk: critical for regressions but not a first containment target. These files
are central UI infrastructure and likely depend on long-standing immediate-mode
style assumptions.

Priority: P0 for documentation, P3 for source edits. Do not touch early unless a
small task has a clear test surface.

## Boundary Observations

- UI rendering is not isolated from the legacy GL layer. Even core `llui` uses
  `gGL`, `LLFontGL`, `LLUIImage`, and direct scissor state.
- Preview UI is more coupled than ordinary widgets because it renders scene,
  material, model, or capture content into viewer textures.
- Dynamic preview rendering borrows render targets from `gPipeline`; it is not a
  standalone UI texture renderer.
- Map widgets are UI controls that also perform world-space or map-space
  rendering directly.
- Some raw inventory matches in UI are false positives, especially helper names
  in `LLUI`.

## Areas Not To Modify First

- `LLViewerDynamicTexture::updateAllInstances()` in `lldynamictexture.cpp`
- `LLScreenClipRect::updateScissorRegion()` in `lllocalcliprect.cpp`
- `LLView::draw()`, `LLView::drawChildren()`, and `LLView::drawChild()`
- `LLModelPreview::render()` in `llmodelpreview.cpp`
- `LLGLTFPreviewTexture::render()` in `llgltfmaterialpreviewmgr.cpp`
- `LLNetMap::draw()` and `LLWorldMapView::draw()`
- texture/material preview entry points in `lltexturectrl.cpp`

Reason: these areas combine UI traversal, render target ownership, texture
lifetime, matrix state, shader state, and direct GL state. They are better used
as mapping targets until the phase 1 evidence is complete.

## Small Follow-Up Tasks

- Add a compact call-flow note for `LLViewerDynamicTexture::updateAllInstances()`.
- List all subclasses or direct users of `LLViewerDynamicTexture`.
- Split map rendering notes into minimap, world map, and tracking overlay
  responsibilities.
- Add a review checklist item for any new UI code that touches `gGL`,
  `LLRenderTarget`, `LLViewerTexture`, `LLGL`, or direct `gl*`.
- Add `docs/architecture/08-platform-opengl.md` for Darwin, Windows, SDL, and
  Mesa/headless context glue.
