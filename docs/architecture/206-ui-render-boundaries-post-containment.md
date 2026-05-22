# UI Render Boundaries After GL Containment

Date: 2026-05-22

## Scope

This note updates the UI/render boundary map after runtime `gl*` containment.
It does not replace the phase 1 mapping in
`docs/architecture/07-ui-render-boundaries.md`; that older document remains the
historical baseline.

The current important distinction is:

- runtime OpenGL calls are now routed through `LLGLContainment`;
- UI and preview code still owns render intent through `gGL`, `LLRender`,
  `LLRenderTarget`, `LLViewerTexture`, `LLImageGL`, and pipeline render targets.

So the UI/render boundary is not solved. Only the raw OpenGL call boundary has
been centralized.

## Current Core UI Candidates

From the regenerated inventory, these `indra/llui` files remain the main UI
render boundary candidates:

| file | main signals | reason |
|---|---:|---|
| `indra/llui/llfloater.cpp` | `gGL=23`, `LLGL=1`, `LLRender=1` | floater frame, context cone, image/text drawing |
| `indra/llui/llstatbar.cpp` | `gGL=16`, `LLGL=1`, `LLRender=1` | graph/stat UI draws geometry directly |
| `indra/llui/llview.cpp` | `gGL=14`, `LLRender=2` | UI traversal and debug rectangle drawing |
| `indra/llui/llbadge.cpp` | `gGL=10`, `LLRender=2` | badge image/text geometry |
| `indra/llui/llviewborder.cpp` | `gGL=8` | border geometry |
| `indra/llui/llmenugl.cpp` | `gGL=7` | menu drawing and layout/render coupling |
| `indra/llui/lltabcontainer.cpp` | `gGL=6` | tab arrow transforms and child draw placement |
| `indra/llui/lltextbase.cpp` | `gGL=5` | text selection, cursor, inline image drawing |
| `indra/llui/llbutton.cpp` | `gGL=4`, `LLRender=5` | image/font button drawing and blend state |
| `indra/llui/llscrollbar.cpp` | `gGL=4`, `LLRender=4` | scrollbar geometry and image drawing |
| `indra/llui/llscrolllistctrl.cpp` | `gGL=1`, `LLGL=2` | scroll/list rendering and clipping |
| `indra/llui/lllocalcliprect.cpp` | `gGL=1`, `LLGL=2` | UI scissor ownership and flush ordering |

Risk: high for visual regressions.

Priority: document before source edits. These files should not be a first
target for behavior changes unless the task has a narrow visual surface and a
manual UI smoke path.

## Current Viewer UI Preview Candidates

These `indra/newview` files are not ordinary widgets. They bridge UI controls
with scene-like rendering, render targets, textures, and pipeline state:

| file | main signals | reason |
|---|---:|---|
| `indra/newview/lldynamictexture.cpp` | `gGL=4`, `LLGL=8`, `LLPipeline=4`, `LLRenderTarget=3`, `LLViewerTexture=3` | shared dynamic texture render driver |
| `indra/newview/llmodelpreview.cpp` | `gGL=58`, `LLGL=30`, `LLRender=17` | model upload preview renders scene-like content in UI |
| `indra/newview/llfloaterimagepreview.cpp` | `gGL=72`, `LLGL=8`, `LLRender=11`, `LLDrawPool=2` | image/avatar/sculpt preview rendering |
| `indra/newview/llfloaterbvhpreview.cpp` | `gGL=32`, `LLGL=2`, `LLRender=5`, `LLDrawPool=2` | animation preview rendering |
| `indra/newview/llgltfmaterialpreviewmgr.cpp` | `gGL=2`, `LLGL=48`, `LLPipeline=7`, `LLRenderTarget=1`, `LLViewerTexture=4` | material preview uses pipeline targets and shaders |
| `indra/newview/llsnapshotlivepreview.cpp` | `gGL=64`, `LLGL=3`, `LLRender=3`, `LLViewerTexture=3` | snapshot preview/capture UI boundary |
| `indra/newview/lltexturectrl.cpp` | `LLGL=5`, `LLViewerTexture=5` | texture picker and material preview entry point |
| `indra/newview/llnetmap.cpp` | `gGL=92`, `LLGL=1`, `LLRender=5`, `LLViewerTexture=1` | minimap widget draws map/world overlays directly |
| `indra/newview/llworldmapview.cpp` | `gGL=88`, `LLGL=2`, `LLRender=8` | world map widget draws textures, overlays, and labels |

Risk: high.

Priority: analyze by subsystem, not as generic UI cleanup. Preview renderers
must be treated separately from basic widgets because they use render targets
and pipeline-owned resources.

## Dynamic Texture Users To Map Before Editing

Known `LLViewerDynamicTexture` users found by source search:

- `LLModelPreview`
- `LLImagePreviewAvatar`
- `LLImagePreviewSculpted`
- `LLViewerTexLayerSetBuffer`
- `LLVisualParamHint`
- `LLVisualParamReset`
- `LLGLTFPreviewTexture`
- `LLPreviewAnimation`

Before changing `LLViewerDynamicTexture::updateAllInstances()`, map which of
these users can render during login, avatar editing, upload preview, material
preview, snapshot, or normal scene rendering.

## Current Guardrail

Do not treat "no direct `gl*` calls in UI files" as a UI/render decoupling.
The remaining boundary is higher level:

- matrix and immediate drawing via `gGL`;
- render-target borrowing through dynamic textures;
- texture lifetime through `LLViewerTexture` and `LLImageGL`;
- pipeline state through preview managers;
- clipping and scissor ownership through `LLLocalClipRect`.

## Small Next Tasks

- Add a call-flow note for `LLViewerDynamicTexture::updateAllInstances()`.
- Split map UI rendering into minimap, world map, and tracking overlay notes.
- Add a focused task before changing `LLLocalClipRect` behavior.
- Add a focused task before changing model, image, animation, or GLTF preview
  rendering.
- Keep basic widget rendering changes separate from dynamic preview rendering.
