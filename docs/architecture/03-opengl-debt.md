# OpenGL Debt

Source: `docs/architecture/generated/source_inventory.csv`.

This document summarizes the current OpenGL debt visible from the generated
source inventory. It is a phase 1 analysis artifact only. It does not propose
runtime behavior changes and does not imply that any listed file should be
edited early.

## Summary

- Total files analyzed: 3083
- Files with direct `gl*` calls: 79
- Share of analyzed files with direct `gl*` calls: about 2.6%

Interpretation: direct OpenGL calls are concentrated in a small number of
files, but those files include the central rendering pipeline, low-level GL
wrappers, texture upload code, shader management, draw pools, UI-adjacent
preview surfaces, and platform window code.

## Top 50 Files By Direct `gl*` Calls

| # | file | category | lines | gl_calls | gGL | LLGL | LLRenderTarget |
|---:|---|---|---:|---:|---:|---:|---:|
| 1 | `indra/llrender/llgl.cpp` | render.legacy_low_level | 2949 | 2213 | 31 | 79 | 0 |
| 2 | `indra/llrender/llglheaders.h` | render.legacy_low_level | 1076 | 762 | 0 | 1 | 0 |
| 3 | `indra/llrender/llimagegl.cpp` | render.legacy_low_level | 2651 | 84 | 32 | 21 | 0 |
| 4 | `indra/newview/pipeline.cpp` | render.pipeline | 12046 | 83 | 340 | 158 | 60 |
| 5 | `indra/newview/marefsr2upscaler.cpp` | render.opengl_touching | 379 | 81 | 3 | 3 | 4 |
| 6 | `indra/llrender/llglslshader.cpp` | render.legacy_low_level | 2091 | 80 | 11 | 116 | 2 |
| 7 | `indra/newview/lldrawpoolterrain.cpp` | render.pipeline | 1123 | 62 | 189 | 26 | 0 |
| 8 | `indra/llrender/llvertexbuffer.cpp` | render.legacy_low_level | 1912 | 46 | 33 | 10 | 0 |
| 9 | `indra/llrender/llrender.cpp` | render.legacy_low_level | 2161 | 45 | 39 | 21 | 2 |
| 10 | `indra/llrender/llrendertarget.cpp` | render.legacy_low_level | 585 | 42 | 12 | 2 | 34 |
| 11 | `indra/llrender/llshadermgr.cpp` | render.legacy_low_level | 1563 | 39 | 0 | 4 | 0 |
| 12 | `indra/newview/llspatialpartition.cpp` | render.pipeline | 4188 | 27 | 206 | 28 | 0 |
| 13 | `indra/llrender/llgl.h` | render.legacy_low_level | 485 | 26 | 0 | 62 | 0 |
| 14 | `indra/llrender/llpostprocess.cpp` | render.legacy_low_level | 455 | 20 | 18 | 1 | 0 |
| 15 | `indra/newview/llviewerdisplay.cpp` | render.pipeline | 1953 | 19 | 129 | 37 | 1 |
| 16 | `indra/newview/llmodelpreview.cpp` | render.opengl_touching | 4172 | 17 | 58 | 13 | 0 |
| 17 | `indra/newview/gltfscenemanager.cpp` | render.pipeline | 1210 | 17 | 56 | 21 | 0 |
| 18 | `indra/newview/llreflectionmapmanager.cpp` | render.pipeline | 1605 | 15 | 69 | 4 | 1 |
| 19 | `indra/newview/llviewerwindow.cpp` | render.pipeline | 6504 | 14 | 44 | 7 | 2 |
| 20 | `indra/newview/llmaniptranslate.cpp` | render.pipeline | 2323 | 13 | 196 | 22 | 0 |
| 21 | `indra/llwindow/llwindowsdl.cpp` | assets.texture | 2757 | 12 | 0 | 0 | 0 |
| 22 | `indra/newview/llscenemonitor.cpp` | render.pipeline | 757 | 11 | 26 | 3 | 8 |
| 23 | `indra/newview/llface.cpp` | render.pipeline | 2638 | 9 | 15 | 13 | 0 |
| 24 | `indra/newview/lldrawpoolmaterials.cpp` | render.draw_pool | 290 | 8 | 11 | 0 | 0 |
| 25 | `indra/newview/gltf/asset.cpp` | assets.texture | 1470 | 8 | 0 | 5 | 0 |
| 26 | `indra/newview/llreflectionmap.cpp` | render.pipeline | 383 | 8 | 0 | 3 | 0 |
| 27 | `indra/newview/llvieweroctree.cpp` | render.pipeline | 1551 | 7 | 0 | 4 | 0 |
| 28 | `indra/newview/llvoavatar.cpp` | render.pipeline | 12711 | 6 | 96 | 8 | 0 |
| 29 | `indra/newview/llselectmgr.cpp` | render.pipeline | 9001 | 6 | 68 | 48 | 0 |
| 30 | `indra/newview/llglsandbox.cpp` | render.pipeline | 1130 | 5 | 129 | 19 | 2 |
| 31 | `indra/llappearance/lltexlayer.cpp` | render.opengl_touching | 1931 | 5 | 85 | 17 | 10 |
| 32 | `indra/newview/gltf/animation.cpp` | render.opengl_touching | 490 | 5 | 0 | 0 | 0 |
| 33 | `indra/llrender/llrender2dutils.cpp` | render.legacy_low_level | 1871 | 4 | 577 | 0 | 2 |
| 34 | `indra/newview/llfasttimerview.cpp` | render.opengl_touching | 1668 | 4 | 75 | 4 | 1 |
| 35 | `indra/llrender/llcubemap.cpp` | render.legacy_low_level | 344 | 4 | 21 | 0 | 0 |
| 36 | `indra/llrender/llcubemaparray.cpp` | render.legacy_low_level | 220 | 4 | 2 | 0 | 0 |
| 37 | `indra/llrender/llglstates.h` | render.legacy_low_level | 197 | 4 | 0 | 46 | 0 |
| 38 | `indra/newview/llappviewer.cpp` | render.pipeline | 6743 | 4 | 0 | 1 | 4 |
| 39 | `indra/llwindow/llwindowwin32.cpp` | render.opengl_touching | 5207 | 4 | 0 | 0 | 0 |
| 40 | `indra/llui/llui.cpp` | render.opengl_touching | 764 | 4 | 0 | 0 | 0 |
| 41 | `indra/newview/llsnapshotlivepreview.cpp` | assets.texture | 1104 | 3 | 64 | 0 | 0 |
| 42 | `indra/newview/llheroprobemanager.cpp` | render.pipeline | 661 | 3 | 22 | 3 | 2 |
| 43 | `indra/newview/llhudeffectlookat.cpp` | render.opengl_touching | 826 | 3 | 16 | 1 | 0 |
| 44 | `indra/newview/llterrainpaintmap.cpp` | assets.texture | 287 | 3 | 15 | 8 | 1 |
| 45 | `indra/newview/llhudeffectpointat.cpp` | render.opengl_touching | 523 | 3 | 15 | 1 | 0 |
| 46 | `indra/newview/llviewerjointmesh.cpp` | render.draw_pool | 532 | 3 | 12 | 4 | 0 |
| 47 | `indra/newview/lldynamictexture.cpp` | render.pipeline | 308 | 3 | 4 | 5 | 3 |
| 48 | `indra/newview/llgltfmaterialpreviewmgr.cpp` | render.pipeline | 591 | 3 | 2 | 45 | 1 |
| 49 | `indra/llrender/llimagegl.h` | render.legacy_low_level | 369 | 3 | 0 | 25 | 0 |
| 50 | `indra/llwindow/llwindowmacosx-objc.h` | render.opengl_touching | 187 | 3 | 0 | 0 | 0 |

## Top 20 Files By `gGL`

| # | file | category | lines | gGL | gl_calls | LLGL | LLRenderTarget |
|---:|---|---|---:|---:|---:|---:|---:|
| 1 | `indra/llrender/llrender2dutils.cpp` | render.legacy_low_level | 1871 | 577 | 4 | 0 | 2 |
| 2 | `indra/newview/pipeline.cpp` | render.pipeline | 12046 | 340 | 83 | 158 | 60 |
| 3 | `indra/newview/llspatialpartition.cpp` | render.pipeline | 4188 | 206 | 27 | 28 | 0 |
| 4 | `indra/newview/llmaniptranslate.cpp` | render.pipeline | 2323 | 196 | 13 | 22 | 0 |
| 5 | `indra/newview/lldrawpoolterrain.cpp` | render.pipeline | 1123 | 189 | 62 | 26 | 0 |
| 6 | `indra/newview/llviewerdisplay.cpp` | render.pipeline | 1953 | 129 | 19 | 37 | 1 |
| 7 | `indra/newview/llglsandbox.cpp` | render.pipeline | 1130 | 129 | 5 | 19 | 2 |
| 8 | `indra/newview/llvoavatar.cpp` | render.pipeline | 12711 | 96 | 6 | 8 | 0 |
| 9 | `indra/newview/llnetmap.cpp` | assets.texture | 1804 | 92 | 1 | 0 | 0 |
| 10 | `indra/newview/llworldmapview.cpp` | render.opengl_touching | 1983 | 88 | 0 | 2 | 0 |
| 11 | `indra/llappearance/lltexlayer.cpp` | render.opengl_touching | 1931 | 85 | 5 | 17 | 10 |
| 12 | `indra/newview/llmanipscale.cpp` | render.opengl_touching | 2079 | 78 | 2 | 11 | 0 |
| 13 | `indra/newview/llfasttimerview.cpp` | render.opengl_touching | 1668 | 75 | 4 | 4 | 1 |
| 14 | `indra/newview/llfloaterimagepreview.cpp` | render.draw_pool | 1089 | 72 | 1 | 7 | 0 |
| 15 | `indra/newview/llmaniprotate.cpp` | render.opengl_touching | 1976 | 70 | 0 | 13 | 0 |
| 16 | `indra/newview/llreflectionmapmanager.cpp` | render.pipeline | 1605 | 69 | 15 | 4 | 1 |
| 17 | `indra/newview/llselectmgr.cpp` | render.pipeline | 9001 | 68 | 6 | 48 | 0 |
| 18 | `indra/newview/llsnapshotlivepreview.cpp` | assets.texture | 1104 | 64 | 3 | 0 | 0 |
| 19 | `indra/newview/llmodelpreview.cpp` | render.opengl_touching | 4172 | 58 | 17 | 13 | 0 |
| 20 | `indra/newview/gltfscenemanager.cpp` | render.pipeline | 1210 | 56 | 17 | 21 | 0 |

## Top 20 Files By `LLRenderTarget`

| # | file | category | lines | LLRenderTarget | gl_calls | gGL | LLGL |
|---:|---|---|---:|---:|---:|---:|---:|
| 1 | `indra/newview/pipeline.cpp` | render.pipeline | 12046 | 60 | 83 | 340 | 158 |
| 2 | `indra/newview/pipeline.h` | render.pipeline | 1133 | 53 | 0 | 0 | 11 |
| 3 | `indra/llrender/llrendertarget.cpp` | render.legacy_low_level | 585 | 34 | 42 | 12 | 2 |
| 4 | `indra/llrender/llrendertarget.h` | render.legacy_low_level | 195 | 14 | 0 | 1 | 1 |
| 5 | `indra/llappearance/lltexlayer.h` | render.opengl_touching | 314 | 12 | 0 | 0 | 2 |
| 6 | `indra/llappearance/lltexlayer.cpp` | render.opengl_touching | 1931 | 10 | 5 | 85 | 17 |
| 7 | `indra/newview/marenisupscaler.h` | viewer.misc | 77 | 9 | 0 | 0 | 0 |
| 8 | `indra/newview/llscenemonitor.cpp` | render.pipeline | 757 | 8 | 11 | 26 | 3 |
| 9 | `indra/newview/marenisupscaler.cpp` | render.opengl_touching | 135 | 8 | 0 | 9 | 4 |
| 10 | `indra/newview/llvisualeffect.h` | viewer.misc | 186 | 6 | 0 | 0 | 0 |
| 11 | `indra/newview/marefsr2upscaler.h` | render.opengl_touching | 104 | 5 | 2 | 0 | 1 |
| 12 | `indra/newview/llscenemonitor.h` | assets.texture | 130 | 5 | 1 | 0 | 0 |
| 13 | `indra/newview/mareupscaler.h` | viewer.misc | 68 | 5 | 0 | 0 | 0 |
| 14 | `indra/newview/maretaaupscaler.h` | viewer.misc | 55 | 5 | 0 | 0 | 0 |
| 15 | `indra/newview/marefsr2upscaler.cpp` | render.opengl_touching | 379 | 4 | 81 | 3 | 3 |
| 16 | `indra/newview/llappviewer.cpp` | render.pipeline | 6743 | 4 | 4 | 0 | 1 |
| 17 | `indra/newview/maretaaupscaler.cpp` | render.opengl_touching | 181 | 4 | 2 | 15 | 5 |
| 18 | `indra/newview/lldynamictexture.cpp` | render.pipeline | 308 | 3 | 3 | 4 | 5 |
| 19 | `indra/newview/lldrawpoolwater.cpp` | render.pipeline | 339 | 3 | 0 | 5 | 4 |
| 20 | `indra/llrender/llglslshader.cpp` | render.legacy_low_level | 2091 | 2 | 80 | 11 | 116 |

## Where OpenGL Is Probably Mixed With UI

Hypothesis: OpenGL is probably mixed with UI in three recurring patterns.

1. Preview widgets and floaters render scene/model/texture content directly:
   - `indra/newview/llmodelpreview.cpp`
   - `indra/newview/llfloaterimagepreview.cpp`
   - `indra/newview/llsnapshotlivepreview.cpp`
   - `indra/newview/llgltfmaterialpreviewmgr.cpp`
   - `indra/newview/llfloatermodelpreview.cpp`

2. UI surfaces or map-like widgets use `gGL` or texture/render helpers:
   - `indra/newview/llnetmap.cpp`
   - `indra/newview/llworldmapview.cpp`
   - `indra/newview/llviewerwindow.cpp`
   - `indra/newview/llpanelface.cpp`
   - `indra/newview/lltexturectrl.cpp`

3. Core UI framework files still touch rendering primitives:
   - `indra/llui/llui.cpp`
   - `indra/llui/llui.h`
   - `indra/llui/lltextbase.cpp`
   - `indra/llui/llmenugl.cpp`
   - `indra/llui/lllocalcliprect.cpp`
   - `indra/llui/llscrolllistctrl.cpp`
   - `indra/llui/lltexteditor.cpp`
   - `indra/llui/llview.cpp`
   - `indra/llui/llbutton.cpp`
   - `indra/llui/llfloater.cpp`

These are not necessarily bugs. They indicate boundary debt: UI code likely
depends on immediate rendering state, texture objects, or viewer render
helpers instead of a narrow UI rendering interface.

## OpenGL Containment Strategy

The containment strategy should stay compatible with phase 1 rules: no file
moves, no runtime behavior changes, no direct Vulkan port, and no broad GL
replacement.

1. Keep `indra/llrender/` as the legacy OpenGL boundary for now.
   - Treat `llgl*`, `llrender*`, `llrendertarget*`, `llvertexbuffer*`,
     `llglslshader*`, and `llshadermgr*` as the current low-level GL zone.
   - Do not add new direct `gl*` calls outside this zone.
   - Do not rewrite this zone until call sites and contracts are documented.

2. Document every direct `gl*` call outside `indra/llrender/`.
   - Start with the 79 files with `gl_calls > 0`.
   - Mark each call group by intent: state, buffer, texture, shader,
     framebuffer, draw, debug, or platform.
   - Keep the result in `docs/architecture/`.

3. Split ownership conceptually before changing code.
   - Frame orchestration: `pipeline.cpp`, `llviewerdisplay.cpp`,
     `llviewerwindow.cpp`.
   - Resource ownership: textures, shaders, render targets, vertex buffers.
   - Render passes: draw pools, reflection, terrain, avatar, water, GLTF.
   - UI rendering: preview widgets, map widgets, core `llui`.

4. Contain optional or platform-incompatible backends first.
   - `marefsr2upscaler.cpp` is already a concrete example: it contains modern
     OpenGL compute calls and is disabled on Darwin.
   - Future optional backends should have explicit build flags and clear
     fallback behavior.

5. Add documentation-only review gates.
   - New rendering work should identify whether it adds direct `gl*`, `gGL`,
     `LLGL`, `LLRenderTarget`, or `LLViewerTexture` usage.
   - Any new direct OpenGL dependency outside `indra/llrender/` should require
     an explicit justification.

6. Delay abstraction until evidence is stronger.
   - Do not introduce a renderer interface until the top call sites are
     classified by intent.
   - Prioritize measurement and inventory over mechanical wrappers.

## Small Follow-Up Tasks

- Generate a direct `gl*` callsite inventory grouped by function name.
- Add a UI/render boundary candidate list for preview and map widgets.
- Add a render target lifecycle map for `pipeline.cpp` and `llrendertarget.cpp`.
- Add a shader-family map for `llviewershadermgr.cpp` and
  `app_settings/shaders`.
- Add a platform OpenGL note for Darwin, Windows, SDL, and Mesa/headless files.

