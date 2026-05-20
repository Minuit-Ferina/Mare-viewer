# OpenGL Callsite Inventory

Source: `docs/architecture/generated/source_inventory.csv`, regenerated after
`indra/llrender/llglcontainment.*` landed.

This is a phase 1 analysis artifact only. It does not propose edits to the
listed source files.

## Method

The generated CSV counts raw `gl*` references with this regex:
`\bgl[A-Z][A-Za-z0-9_]*\b`.

For this document, a second heuristic pass was used to separate likely direct
call expressions from comments, declarations, URLs, and names:
`\b(gl[A-Z][A-Za-z0-9_]*)\s*\(`.

Known false-positive patterns in the raw inventory:

- `glTF` in comments, URLs, shader comments, and GLTF-related identifiers.
- `glPointToScreen` and `glRectToScreen` in `LLUI`; these are coordinate helper
  names, not OpenGL API calls.
- `glReady` in the Windows window thread; this is an internal method name.
- `glView` in macOS Objective-C bridge declarations.
- `glQuery` in comments.

Because the inventory is regex-based, treat every row below as a classification
target, not as proof that a file should be changed.

## Summary

- Total files analyzed: 3085
- Files with raw `gl*` references: 79
- Raw `gl*` references: 3900
- Files with likely `gl*(...)` call expressions: 69
- Likely `gl*(...)` call expressions: 945
- Files with raw `gl*` references outside `indra/llrender/`: 63
- Files with likely `gl*(...)` call expressions outside `indra/llrender/`: 53

Interpretation: `indra/llrender/` remains the current low-level OpenGL boundary,
but substantial direct GL usage still exists in frame orchestration, draw pools,
reflection probes, preview widgets, UI-adjacent rendering, and platform window
glue.

## Function Families By Intent

| intent | representative functions | containment note |
|---|---|---|
| state and fixed-function draw state | `glEnable`, `glDisable`, `glCullFace`, `glPolygonMode`, `glLineWidth`, `glPointSize`, `glPolygonOffset` | Highest risk when mixed into draw pools and UI previews because state can leak across passes. |
| framebuffer and render targets | `glBindFramebuffer`, `glFramebufferTexture2D`, `glCheckFramebufferStatus`, `glCopyTexSubImage2D`, `glCopyTexSubImage3D`, `glReadPixels` | Should be mapped with `LLRenderTarget` lifecycle before any abstraction. |
| textures and image binding | `glBindTexture`, `glBindTextureUnit`, `glTexParameteri`, `glTexImage2D`, `glGenerateMipmap`, `glPixelStorei`, `glBindImageTexture` | Tied to `LLImageGL`, `LLViewerTexture`, probes, terrain, previews, and upscalers. |
| shaders, uniforms, and compute | `glCreateShader`, `glCompileShader`, `glUseProgram`, `glGetUniformLocation`, `glUniform*`, `glDispatchCompute`, `glMemoryBarrier` | `marefsr2upscaler.cpp` is the main optional modern-GL island and is already disabled on Darwin. |
| buffers and geometry | `glBindBuffer`, `glBufferData`, `glBufferSubData`, `glVertexAttribPointer`, `glVertexPointer`, `glDrawElements` | Crosses GLTF resources, spatial debug rendering, and legacy draw paths. |
| queries and debug timing | `glBeginQuery`, `glEndQuery`, `glGetQueryObjectuiv`, `glGetQueryObjectui64v`, `glObjectLabel` | Useful for measurement, but should be isolated from render pass ownership decisions. |
| platform and context glue | `glXGetProcAddressARB`, `glGetString`, `glFinish`, `glSwapBuffers` | Likely belongs under platform windowing notes, not renderer refactor work. |
| raw-inventory false positives | `glTF`, `glReady`, `glPointToScreen`, `glRectToScreen`, `glView`, comment-only `glQuery` | Keep these in the inventory until the generator is refined. |

## Top Likely Call Expressions

| # | function | count | primary intent |
|---:|---|---:|---|
| 1 | `glLineWidth` | 41 | state/debug drawing |
| 2 | `glPolygonMode` | 39 | state/debug drawing |
| 3 | `glGetIntegerv` | 37 | state/platform query |
| 4 | `glTexParameteri` | 29 | texture state |
| 5 | `glClear` | 27 | frame/render target clear |
| 6 | `glClearColor` | 25 | frame/render target clear |
| 7 | `glGetError` | 25 | debug/error handling |
| 8 | `glBindBuffer` | 23 | buffer binding |
| 9 | `glEnable` | 23 | state |
| 10 | `glBindFramebuffer` | 21 | render target |
| 11 | `glGetString` | 19 | platform/capability query |
| 12 | `glGetUniformLocation` | 19 | shader uniform |
| 13 | `glPolygonOffset` | 19 | state/depth bias |
| 14 | `glViewport` | 18 | frame/render target viewport |
| 15 | `glDisable` | 17 | state |
| 16 | `glTexGenfv` | 16 | fixed-function texture generation |
| 17 | `glTexGeni` | 16 | fixed-function texture generation |
| 18 | `glPixelStorei` | 15 | texture upload state |
| 19 | `glBindTexture` | 14 | texture binding |
| 20 | `glBindTextureUnit` | 13 | texture binding |
| 21 | `glVertexAttribPointer` | 13 | vertex format |
| 22 | `glCullFace` | 11 | state |
| 23 | `glGenerateMipmap` | 10 | texture lifecycle |
| 24 | `glUniform1f` | 10 | shader uniform |
| 25 | `glBeginQuery` | 8 | query/timing |
| 26 | `glBufferData` | 8 | buffer upload |
| 27 | `glEndQuery` | 8 | query/timing |
| 28 | `glGenBuffers` | 8 | buffer lifecycle |
| 29 | `glStencilFunc` | 8 | stencil state |
| 30 | `glUseProgram` | 8 | shader program binding |

## Direct GL Outside `indra/llrender/`

Priority means analysis priority only:

- `P0`: map before touching related rendering behavior.
- `P1`: classify before source changes in the area.
- `P2`: lower-volume or boundary-cleanup candidate.

| file | likely calls | raw refs | disposition | priority | representative symbols |
|---|---:|---:|---|---|---|
| `indra/newview/marefsr2upscaler.cpp` | 81 | 81 | optional shader/program/compute backend | P0 | `glCreateShader`, `glShaderSource`, `glCompileShader`, `glGetShaderiv`, `glGetShaderInfoLog`, `glDeleteShader`, `glCreateProgram`, `glAttachShader` |
| `indra/newview/pipeline.cpp` | 81 | 83 | frame orchestration, render targets, state, queries | P0 | `glDeleteQueries`, `glTexParameteri`, `glClearColor`, `glClear`, `glGenQueries`, `glBeginQuery`, `glEndQuery`, `glStencilFunc` |
| `indra/newview/lldrawpoolterrain.cpp` | 61 | 62 | terrain draw state and fixed-function texture generation | P0 | `glCullFace`, `glPolygonOffset`, `glEnable`, `glTexGeni`, `glTexGenfv`, `glDisable` |
| `indra/newview/llspatialpartition.cpp` | 27 | 27 | spatial/debug geometry | P1 | `glPolygonMode`, `glLineWidth`, `glVertexPointer`, `glDrawElements`, `glPolygonOffset`, `glMatrixMode`, `glPushMatrix`, `glLoadIdentity` |
| `indra/newview/llviewerdisplay.cpp` | 19 | 19 | frame clear, viewport, and display state | P0 | `glClear`, `glClearColor`, `glViewport`, `glColorMask`, `glPolygonMode` |
| `indra/newview/gltfscenemanager.cpp` | 17 | 17 | GLTF scene resource binding and draw state | P1 | `glBindBufferBase`, `glActiveTexture`, `glBindTexture`, `glTexParameteri`, `glPolygonMode` |
| `indra/newview/llmodelpreview.cpp` | 17 | 17 | UI/model preview rendering boundary | P1 | `glLineWidth`, `glPolygonMode`, `glClear`, `glPointSize` |
| `indra/newview/llreflectionmapmanager.cpp` | 15 | 15 | reflection probe texture and buffer lifecycle | P1 | `glTexImage2D`, `glGenerateMipmap`, `glCopyTexSubImage3D`, `glViewport`, `glGenBuffers`, `glBindBuffer`, `glBufferData`, `glBindBufferBase` |
| `indra/newview/llmaniptranslate.cpp` | 13 | 13 | UI/world manipulator stencil state | P1 | `glStencilMask`, `glClearStencil`, `glClear`, `glStencilFunc`, `glStencilOp`, `glCullFace` |
| `indra/newview/llviewerwindow.cpp` | 12 | 14 | window/display readback and viewport handling | P0 | `glReadPixels`, `glViewport`, `glCullFace`, `glClear` |
| `indra/llwindow/llwindowsdl.cpp` | 11 | 12 | platform/context glue | P1 | `glXGetProcAddressARB`, `glGetIntegerv`, `glGetString`, `glDisable` |
| `indra/newview/llscenemonitor.cpp` | 11 | 11 | debug render target and query handling | P1 | `glBindFramebuffer`, `glCopyTexSubImage2D`, `glColorMask`, `glBeginQuery`, `glEndQuery`, `glGetQueryObjectuiv` |
| `indra/newview/llface.cpp` | 9 | 9 | geometry draw path | P2 | `glPolygonOffset`, `glVertexPointer`, `glEnableClientState`, `glTexCoordPointer`, `glDrawElements`, `glDisableClientState`, `glLineWidth`, `glPolygonMode` |
| `indra/newview/gltf/asset.cpp` | 8 | 8 | GLTF buffer upload | P2 | `glGenBuffers`, `glBindBuffer`, `glBufferData` |
| `indra/newview/lldrawpoolmaterials.cpp` | 8 | 8 | material shader uniforms | P2 | `glUniform1f`, `glUniform4fv` |
| `indra/newview/llreflectionmap.cpp` | 6 | 8 | reflection query handling | P2 | `glDeleteQueries`, `glGenQueries`, `glGetQueryObjectuiv`, `glBeginQuery`, `glEndQuery` |
| `indra/newview/llvoavatar.cpp` | 6 | 6 | avatar debug/query drawing | P2 | `glLineWidth`, `glGenQueries`, `glBeginQuery`, `glEndQuery`, `glGetQueryObjectui64v` |
| `indra/newview/gltf/animation.cpp` | 5 | 5 | GLTF animation buffer upload | P2 | `glDeleteBuffers`, `glGenBuffers`, `glBindBuffer`, `glBufferData` |
| `indra/newview/llglsandbox.cpp` | 5 | 5 | debug drawing state | P2 | `glLineWidth` |
| `indra/newview/llselectmgr.cpp` | 5 | 6 | selection drawing state | P2 | `glAlphaFunc`, `glPolygonMode`, `glLineWidth` |
| `indra/newview/llvieweroctree.cpp` | 5 | 7 | octree query/debug handling | P2 | `glGenQueries`, `glGetQueryObjectuiv`, `glBeginQuery`, `glEndQuery` |
| `indra/llui/llui.cpp` | 4 | 4 | UI coordinate helpers, false-positive candidate | P2 | `glPointToScreen`, `glRectToScreen` |
| `indra/llwindow/llwindowwin32.cpp` | 4 | 4 | platform/context glue plus clear call | P1 | `glReady`, `glClearColor`, `glClear` |
| `indra/newview/llappviewer.cpp` | 4 | 4 | startup/capability and cleanup touchpoint | P2 | `glGetString`, `glDeleteTextures` |
| `indra/llappearance/lltexlayer.cpp` | 3 | 5 | appearance texture readback | P2 | `glGetTexImage`, `glGetError`, `glReadPixels` |
| `indra/newview/lldynamictexture.cpp` | 3 | 3 | dynamic texture render target state | P2 | `glViewport`, `glClear` |
| `indra/newview/llfasttimerview.cpp` | 3 | 4 | profiler UI readback/debug drawing | P2 | `glReadPixels`, `glLineWidth` |
| `indra/newview/llgltfmaterialpreviewmgr.cpp` | 3 | 3 | UI/material preview rendering boundary | P2 | `glClearColor`, `glClear` |
| `indra/newview/llheroprobemanager.cpp` | 3 | 3 | probe texture copy and viewport | P2 | `glCopyTexSubImage3D`, `glViewport` |
| `indra/newview/llhudeffectlookat.cpp` | 3 | 3 | HUD fixed-function matrix state | P2 | `glMatrixMode`, `glPushMatrix`, `glPopMatrix` |
| `indra/newview/llhudeffectpointat.cpp` | 3 | 3 | HUD fixed-function matrix state | P2 | `glMatrixMode`, `glPushMatrix`, `glPopMatrix` |
| `indra/newview/llsnapshotlivepreview.cpp` | 3 | 3 | UI/snapshot preview rendering boundary | P2 | `glGetFloatv`, `glLineWidth` |
| `indra/newview/llterrainpaintmap.cpp` | 3 | 3 | terrain paint texture lifecycle | P2 | `glClearColor`, `glViewport`, `glGenerateMipmap` |
| `indra/llui/llui.h` | 2 | 2 | UI coordinate helpers, false-positive candidate | P2 | `glPointToScreen`, `glRectToScreen` |
| `indra/newview/RRInterface.cpp` | 2 | 2 | viewer draw state touchpoint | P2 | `glCullFace` |
| `indra/newview/lldrawpoolbump.cpp` | 2 | 2 | draw pool state and mip lifecycle | P2 | `glPolygonOffset`, `glGenerateMipmap` |
| `indra/newview/lldrawpooltree.cpp` | 2 | 2 | draw pool state | P2 | `glPolygonOffset` |
| `indra/newview/llmanipscale.cpp` | 2 | 2 | UI/world manipulator draw state | P2 | `glPolygonOffset` |
| `indra/newview/llviewerjoint.cpp` | 2 | 2 | avatar joint draw state | P2 | `glCullFace` |
| `indra/newview/maretaaupscaler.cpp` | 2 | 2 | local upscaler clear path | P2 | `glClearColor`, `glClear` |
| `indra/llcommon/llprofiler.h` | 1 | 1 | debug object labeling | P2 | `glObjectLabel` |
| `indra/llui/lllocalcliprect.cpp` | 1 | 1 | UI clipping boundary | P2 | `glScissor` |
| `indra/llwindow/llwindowmacosx-objc.h` | 1 | 3 | platform/context glue | P1 | `glSwapBuffers` |
| `indra/llwindow/llwindowmesaheadless.cpp` | 1 | 1 | platform/headless context glue | P1 | `glFinish` |
| `indra/newview/lldrawpool.cpp` | 1 | 1 | draw pool color state | P2 | `glColor4ubv` |
| `indra/newview/lldrawpoolsimple.cpp` | 1 | 1 | draw pool state | P2 | `glPolygonOffset` |
| `indra/newview/lldrawpoolwlsky.cpp` | 1 | 1 | sky draw target clear | P2 | `glClear` |
| `indra/newview/llfilepicker.cpp` | 1 | 2 | false-positive candidate | P2 | `glTF` |
| `indra/newview/llfloaterimagepreview.cpp` | 1 | 1 | UI/image preview rendering boundary | P2 | `glClear` |
| `indra/newview/llnetmap.cpp` | 1 | 1 | UI/map rendering boundary | P2 | `glMatrixMode` |
| `indra/newview/llviewercamera.cpp` | 1 | 1 | camera/viewport touchpoint | P2 | `glViewport` |
| `indra/newview/llviewerparceloverlay.cpp` | 1 | 1 | parcel overlay debug drawing | P2 | `glLineWidth` |
| `indra/newview/rlveffects.cpp` | 1 | 1 | RLV effect viewport touchpoint | P2 | `glViewport` |

## Raw-Only Matches To Filter Later

These files appear in the raw CSV inventory outside `indra/llrender/`, but the
second pass did not find direct `gl*(...)` call expressions. They should remain
visible because some are declarations of GL function pointers, while others are
clearly comments or GLTF references.

| file | raw refs | likely reason |
|---|---:|---|
| `indra/newview/llviewerjointmesh.cpp` | 3 | `extern` declarations for ARB function pointers. |
| `indra/llprimitive/llgltfmaterial.cpp` | 2 | `glTF` in comments/URLs. |
| `indra/newview/app_settings/shaders/class1/deferred/pbrterrainF.glsl` | 2 | `glTF` in comments/URLs. |
| `indra/newview/llviewershadermgr.h` | 2 | `glTF` in comments/URLs. |
| `indra/newview/app_settings/shaders/class1/deferred/textureUtilV.glsl` | 2 | `glTF` in comments/URLs. |
| `indra/newview/marefsr2upscaler.h` | 2 | comment mentioning `glCreateProgram` and `glDispatchCompute`. |
| `indra/newview/llfetchedgltfmaterial.cpp` | 1 | `glTF` in comments. |
| `indra/newview/app_settings/shaders/class2/interface/irradianceGenF.glsl` | 1 | `glTF` in comments/URLs. |
| `indra/newview/app_settings/shaders/class1/interface/radianceGenF.glsl` | 1 | `glTF` in comments/URLs. |
| `indra/newview/llscenemonitor.h` | 1 | comment mentioning `glQuery`. |

## Containment Implications

- Keep `indra/llrender/` as the current legacy OpenGL boundary.
- Do not add new direct `gl*` calls outside `indra/llrender/` without an
  explicit justification.
- Do not mechanically wrap the P0 files yet; first map ownership of frame
  orchestration, render targets, draw pools, and UI preview boundaries.
- Treat platform files under `indra/llwindow/` as platform/context glue until
  `docs/architecture/08-platform-opengl.md` exists.
- Improve the inventory generator before using raw `gl_calls` as a strict gate;
  it should eventually report raw references, likely call expressions, and known
  false positives separately.

## Next Small Tasks

- Add `docs/architecture/05-render-target-lifecycle.md` for `pipeline.cpp` and
  `llrendertarget.cpp`.
- Add `docs/architecture/07-ui-render-boundaries.md` for preview, map, HUD, and
  core `llui` rendering touchpoints.
- Add `docs/architecture/08-platform-opengl.md` for Darwin, Windows, SDL, and
  Mesa/headless context glue.
- Update `tools/architecture/source_inventory.py` only if a later task asks for
  a more precise inventory generator.
