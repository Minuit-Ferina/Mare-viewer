# OpenGL Debt

Source: `docs/architecture/generated/source_inventory.csv`.

This document summarizes the current OpenGL debt visible from the generated
source inventory. It is an architecture analysis artifact only. It does not
propose runtime behavior changes and does not imply that any listed file should
be edited early.

## Summary

- Total files analyzed: 3085
- Files with raw `gl*` references: 79
- Raw `gl*` references: 3900
- Files with likely direct `gl*(...)` calls: 66
- Likely direct `gl*(...)` calls: 936
- Known false-positive raw `gl*` references: 26
- Share of analyzed files with likely direct `gl*(...)` calls: about 2.1%

Interpretation: direct OpenGL call expressions are concentrated in a small
number of files, but those files include the central rendering pipeline,
low-level GL wrappers, texture upload code, shader management, draw pools,
UI-adjacent preview surfaces, and platform window code. The regenerated
inventory keeps raw `gl*` references visible separately so comments, `glTF`,
and helper names do not look like direct OpenGL calls.

## Top 50 Files By Likely Direct `gl*` Calls

| # | file | category | lines | gl_calls | gl_raw_refs | gGL | LLGL | LLRenderTarget |
|---:|---|---|---:|---:|---:|---:|---:|---:|
| 1 | `indra/newview/pipeline.cpp` | render.pipeline | 12046 | 81 | 83 | 340 | 158 | 60 |
| 2 | `indra/newview/marefsr2upscaler.cpp` | render.opengl_touching | 379 | 81 | 81 | 3 | 3 | 4 |
| 3 | `indra/llrender/llglslshader.cpp` | render.legacy_low_level | 2091 | 80 | 80 | 11 | 116 | 2 |
| 4 | `indra/llrender/llgl.cpp` | render.legacy_low_level | 2949 | 79 | 2213 | 31 | 79 | 0 |
| 5 | `indra/llrender/llimagegl.cpp` | render.legacy_low_level | 2651 | 71 | 84 | 32 | 21 | 0 |
| 6 | `indra/newview/lldrawpoolterrain.cpp` | render.pipeline | 1123 | 61 | 62 | 189 | 26 | 0 |
| 7 | `indra/llrender/llrender.cpp` | render.legacy_low_level | 2161 | 44 | 45 | 39 | 21 | 2 |
| 8 | `indra/llrender/llrendertarget.cpp` | render.legacy_low_level | 585 | 42 | 42 | 12 | 2 | 34 |
| 9 | `indra/llrender/llvertexbuffer.cpp` | render.legacy_low_level | 1912 | 40 | 46 | 33 | 10 | 0 |
| 10 | `indra/llrender/llglheaders.h` | render.legacy_low_level | 1076 | 36 | 762 | 0 | 1 | 0 |
| 11 | `indra/llrender/llshadermgr.cpp` | render.legacy_low_level | 1563 | 32 | 39 | 0 | 4 | 0 |
| 12 | `indra/newview/llspatialpartition.cpp` | render.pipeline | 4188 | 27 | 27 | 206 | 28 | 0 |
| 13 | `indra/newview/llviewerdisplay.cpp` | render.pipeline | 1953 | 19 | 19 | 129 | 37 | 1 |
| 14 | `indra/newview/llmodelpreview.cpp` | render.opengl_touching | 4172 | 17 | 17 | 58 | 13 | 0 |
| 15 | `indra/newview/gltfscenemanager.cpp` | render.pipeline | 1210 | 17 | 17 | 56 | 21 | 0 |
| 16 | `indra/newview/llreflectionmapmanager.cpp` | render.pipeline | 1605 | 15 | 15 | 69 | 4 | 1 |
| 17 | `indra/newview/llmaniptranslate.cpp` | render.pipeline | 2323 | 13 | 13 | 196 | 22 | 0 |
| 18 | `indra/llrender/llpostprocess.cpp` | render.legacy_low_level | 455 | 13 | 20 | 18 | 1 | 0 |
| 19 | `indra/newview/llviewerwindow.cpp` | render.pipeline | 6504 | 12 | 14 | 44 | 7 | 2 |
| 20 | `indra/llwindow/llwindowsdl.cpp` | assets.texture | 2757 | 11 | 12 | 0 | 0 | 0 |
| 21 | `indra/newview/llscenemonitor.cpp` | render.pipeline | 757 | 11 | 11 | 26 | 3 | 8 |
| 22 | `indra/newview/llface.cpp` | render.pipeline | 2638 | 9 | 9 | 15 | 13 | 0 |
| 23 | `indra/newview/gltf/asset.cpp` | assets.texture | 1470 | 8 | 8 | 0 | 5 | 0 |
| 24 | `indra/newview/lldrawpoolmaterials.cpp` | render.draw_pool | 290 | 8 | 8 | 11 | 0 | 0 |
| 25 | `indra/newview/llvoavatar.cpp` | render.pipeline | 12711 | 6 | 6 | 96 | 8 | 0 |
| 26 | `indra/newview/llreflectionmap.cpp` | render.pipeline | 383 | 6 | 8 | 0 | 3 | 0 |
| 27 | `indra/newview/llselectmgr.cpp` | render.pipeline | 9001 | 5 | 6 | 68 | 48 | 0 |
| 28 | `indra/newview/llvieweroctree.cpp` | render.pipeline | 1551 | 5 | 7 | 0 | 4 | 0 |
| 29 | `indra/newview/llglsandbox.cpp` | render.pipeline | 1130 | 5 | 5 | 129 | 19 | 2 |
| 30 | `indra/newview/gltf/animation.cpp` | render.opengl_touching | 490 | 5 | 5 | 0 | 0 | 0 |
| 31 | `indra/newview/llappviewer.cpp` | render.pipeline | 6743 | 4 | 4 | 0 | 1 | 4 |
| 32 | `indra/llrender/llcubemaparray.cpp` | render.legacy_low_level | 220 | 4 | 4 | 2 | 0 | 0 |
| 33 | `indra/llrender/llglstates.h` | render.legacy_low_level | 197 | 4 | 4 | 0 | 46 | 0 |
| 34 | `indra/llappearance/lltexlayer.cpp` | render.opengl_touching | 1931 | 3 | 5 | 85 | 17 | 10 |
| 35 | `indra/llrender/llrender2dutils.cpp` | render.legacy_low_level | 1871 | 3 | 4 | 577 | 0 | 2 |
| 36 | `indra/newview/llfasttimerview.cpp` | render.opengl_touching | 1668 | 3 | 4 | 75 | 4 | 1 |
| 37 | `indra/newview/llsnapshotlivepreview.cpp` | assets.texture | 1104 | 3 | 3 | 64 | 0 | 0 |
| 38 | `indra/newview/llhudeffectlookat.cpp` | render.opengl_touching | 826 | 3 | 3 | 16 | 1 | 0 |
| 39 | `indra/newview/llheroprobemanager.cpp` | render.pipeline | 661 | 3 | 3 | 22 | 3 | 2 |
| 40 | `indra/newview/llgltfmaterialpreviewmgr.cpp` | render.pipeline | 591 | 3 | 3 | 2 | 45 | 1 |
| 41 | `indra/newview/llhudeffectpointat.cpp` | render.opengl_touching | 523 | 3 | 3 | 15 | 1 | 0 |
| 42 | `indra/llrender/llimagegl.h` | render.legacy_low_level | 369 | 3 | 3 | 0 | 25 | 0 |
| 43 | `indra/llrender/llcubemap.cpp` | render.legacy_low_level | 344 | 3 | 4 | 21 | 0 | 0 |
| 44 | `indra/newview/lldynamictexture.cpp` | render.pipeline | 308 | 3 | 3 | 4 | 5 | 3 |
| 45 | `indra/newview/llterrainpaintmap.cpp` | assets.texture | 287 | 3 | 3 | 15 | 8 | 1 |
| 46 | `indra/newview/RRInterface.cpp` | render.pipeline | 6995 | 2 | 2 | 14 | 7 | 0 |
| 47 | `indra/llwindow/llwindowwin32.cpp` | render.opengl_touching | 5207 | 2 | 4 | 0 | 0 | 0 |
| 48 | `indra/newview/llmanipscale.cpp` | render.opengl_touching | 2079 | 2 | 2 | 78 | 11 | 0 |
| 49 | `indra/newview/lldrawpoolbump.cpp` | render.pipeline | 1083 | 2 | 2 | 47 | 21 | 1 |
| 50 | `indra/newview/maretaaupscaler.cpp` | render.opengl_touching | 181 | 2 | 2 | 15 | 5 | 4 |

## Top 20 Files By `gGL`

| # | file | category | lines | gGL | gl_calls | gl_raw_refs | LLGL | LLRenderTarget |
|---:|---|---|---:|---:|---:|---:|---:|---:|
| 1 | `indra/llrender/llrender2dutils.cpp` | render.legacy_low_level | 1871 | 577 | 3 | 4 | 0 | 2 |
| 2 | `indra/newview/pipeline.cpp` | render.pipeline | 12046 | 340 | 81 | 83 | 158 | 60 |
| 3 | `indra/newview/llspatialpartition.cpp` | render.pipeline | 4188 | 206 | 27 | 27 | 28 | 0 |
| 4 | `indra/newview/llmaniptranslate.cpp` | render.pipeline | 2323 | 196 | 13 | 13 | 22 | 0 |
| 5 | `indra/newview/lldrawpoolterrain.cpp` | render.pipeline | 1123 | 189 | 61 | 62 | 26 | 0 |
| 6 | `indra/newview/llviewerdisplay.cpp` | render.pipeline | 1953 | 129 | 19 | 19 | 37 | 1 |
| 7 | `indra/newview/llglsandbox.cpp` | render.pipeline | 1130 | 129 | 5 | 5 | 19 | 2 |
| 8 | `indra/newview/llvoavatar.cpp` | render.pipeline | 12711 | 96 | 6 | 6 | 8 | 0 |
| 9 | `indra/newview/llnetmap.cpp` | assets.texture | 1804 | 92 | 1 | 1 | 0 | 0 |
| 10 | `indra/newview/llworldmapview.cpp` | render.opengl_touching | 1983 | 88 | 0 | 0 | 2 | 0 |
| 11 | `indra/llappearance/lltexlayer.cpp` | render.opengl_touching | 1931 | 85 | 3 | 5 | 17 | 10 |
| 12 | `indra/newview/llmanipscale.cpp` | render.opengl_touching | 2079 | 78 | 2 | 2 | 11 | 0 |
| 13 | `indra/newview/llfasttimerview.cpp` | render.opengl_touching | 1668 | 75 | 3 | 4 | 4 | 1 |
| 14 | `indra/newview/llfloaterimagepreview.cpp` | render.draw_pool | 1089 | 72 | 1 | 1 | 7 | 0 |
| 15 | `indra/newview/llmaniprotate.cpp` | render.opengl_touching | 1976 | 70 | 0 | 0 | 13 | 0 |
| 16 | `indra/newview/llreflectionmapmanager.cpp` | render.pipeline | 1605 | 69 | 15 | 15 | 4 | 1 |
| 17 | `indra/newview/llselectmgr.cpp` | render.pipeline | 9001 | 68 | 5 | 6 | 48 | 0 |
| 18 | `indra/newview/llsnapshotlivepreview.cpp` | assets.texture | 1104 | 64 | 3 | 3 | 0 | 0 |
| 19 | `indra/newview/llmodelpreview.cpp` | render.opengl_touching | 4172 | 58 | 17 | 17 | 13 | 0 |
| 20 | `indra/newview/gltfscenemanager.cpp` | render.pipeline | 1210 | 56 | 17 | 17 | 21 | 0 |

## Top 20 Files By `LLRenderTarget`

| # | file | category | lines | LLRenderTarget | gl_calls | gl_raw_refs | gGL | LLGL |
|---:|---|---|---:|---:|---:|---:|---:|---:|
| 1 | `indra/newview/pipeline.cpp` | render.pipeline | 12046 | 60 | 81 | 83 | 340 | 158 |
| 2 | `indra/newview/pipeline.h` | render.pipeline | 1133 | 53 | 0 | 0 | 0 | 11 |
| 3 | `indra/llrender/llrendertarget.cpp` | render.legacy_low_level | 585 | 34 | 42 | 42 | 12 | 2 |
| 4 | `indra/llrender/llrendertarget.h` | render.legacy_low_level | 195 | 14 | 0 | 0 | 1 | 1 |
| 5 | `indra/llappearance/lltexlayer.h` | render.opengl_touching | 314 | 12 | 0 | 0 | 0 | 2 |
| 6 | `indra/llappearance/lltexlayer.cpp` | render.opengl_touching | 1931 | 10 | 3 | 5 | 85 | 17 |
| 7 | `indra/newview/marenisupscaler.h` | viewer.misc | 77 | 9 | 0 | 0 | 0 | 0 |
| 8 | `indra/newview/llscenemonitor.cpp` | render.pipeline | 757 | 8 | 11 | 11 | 26 | 3 |
| 9 | `indra/newview/marenisupscaler.cpp` | render.opengl_touching | 135 | 8 | 0 | 0 | 9 | 4 |
| 10 | `indra/newview/llvisualeffect.h` | viewer.misc | 186 | 6 | 0 | 0 | 0 | 0 |
| 11 | `indra/newview/llscenemonitor.h` | assets.texture | 130 | 5 | 0 | 1 | 0 | 0 |
| 12 | `indra/newview/marefsr2upscaler.h` | render.opengl_touching | 104 | 5 | 0 | 2 | 0 | 1 |
| 13 | `indra/newview/mareupscaler.h` | viewer.misc | 68 | 5 | 0 | 0 | 0 | 0 |
| 14 | `indra/newview/maretaaupscaler.h` | viewer.misc | 55 | 5 | 0 | 0 | 0 | 0 |
| 15 | `indra/newview/marefsr2upscaler.cpp` | render.opengl_touching | 379 | 4 | 81 | 81 | 3 | 3 |
| 16 | `indra/newview/llappviewer.cpp` | render.pipeline | 6743 | 4 | 4 | 4 | 0 | 1 |
| 17 | `indra/newview/maretaaupscaler.cpp` | render.opengl_touching | 181 | 4 | 2 | 2 | 15 | 5 |
| 18 | `indra/newview/lldynamictexture.cpp` | render.pipeline | 308 | 3 | 3 | 3 | 4 | 5 |
| 19 | `indra/newview/lldrawpoolwater.cpp` | render.pipeline | 339 | 3 | 0 | 0 | 5 | 4 |
| 20 | `indra/llrender/llglslshader.cpp` | render.legacy_low_level | 2091 | 2 | 80 | 80 | 11 | 116 |

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

The containment strategy should stay compatible with current architecture
rules: no file moves, no runtime behavior changes without an explicit task, no
direct Vulkan port, and no broad GL replacement.

1. Keep `indra/llrender/` as the legacy OpenGL boundary for now.
   - Treat `llgl*`, `llrender*`, `llrendertarget*`, `llvertexbuffer*`,
     `llglslshader*`, and `llshadermgr*` as the current low-level GL zone.
   - Do not add new direct `gl*` calls outside this zone.
   - Do not rewrite this zone until call sites and contracts are documented.

2. Document every direct `gl*` call outside `indra/llrender/`.
   - Start with the 66 files with `gl_calls > 0`.
   - Keep the 79 files with `gl_raw_refs > 0` visible for false-positive and
     declaration review.
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

- Use `gl_calls` as the initial review gate for future direct-GL containment
  tasks.
- Keep `gl_raw_refs` visible so raw-only files can be reviewed without
  overstating direct OpenGL usage.
- Pick one narrow callsite family and document ownership, ordering, cleanup,
  and verification before adding behavior to `llglcontainment.*`.
