# Viewport Containment Candidate

This document reviews `glViewport` as a possible first OpenGL containment
candidate. It is documentation only and does not propose a source patch yet.

## Inventory

Search:

```sh
rg -n '\bglViewport\s*\(' indra --glob '!build*'
```

Result:

- Active `glViewport(...)` callsites: 17
- Raw matches: 18
- Raw-only match: one commented-out call in `LLViewerWindow::reshape`

## Callsite Groups

| group | files | owner | notes |
|---|---|---|---|
| Render target bind/flush | `indra/llrender/llrendertarget.cpp` | `LLRenderTarget` | `bindTarget()` sets viewport to target size. `flush()` restores the viewer viewport from `gGLViewport` when returning to the default framebuffer. |
| Viewer 2D/3D viewport | `indra/newview/llviewerwindow.cpp`, `indra/newview/llviewercamera.cpp` | viewer window/camera | These calls update `gGLViewport` and represent the canonical viewer viewport. |
| Pipeline post-processing | `indra/newview/pipeline.cpp` | `LLPipeline` | Mixes viewer viewport restore with temporary post-process sizes, especially DoF and antialiasing paths. |
| Probe mip generation | `indra/newview/llreflectionmapmanager.cpp`, `indra/newview/llheroprobemanager.cpp` | probe managers | Temporary cube-map mip viewports. These should not update the viewer viewport. |
| Texture downscale | `indra/llrender/llimagegl.cpp` | `LLImageGL` | Temporary FBO downscale viewport. No local restore is visible in the immediate block, so caller ordering matters. |
| Dynamic textures | `indra/newview/lldynamictexture.cpp` | `LLViewerDynamicTexture` | `preRender()` sets a texture viewport and `postRender()` restores via `gViewerWindow->setup2DViewport()`. |
| Avatar impostors | `indra/newview/llviewerdisplay.cpp` | viewer display/avatar impostor update | Uses a fixed `512 x 512` viewport, then restores through `gViewerWindow->setup3DViewport()`. |
| Terrain paint bake | `indra/newview/llterrainpaintmap.cpp` | terrain paint map | Temporary scratch render target viewport; restore appears to rely on `scratch_target.flush()`. |
| RLV effect fallback | `indra/newview/rlveffects.cpp` | RLV effect pass | Uses destination render target when available; otherwise restores viewer world viewport directly. |

## Current State Model

`gGLViewport` is the global software copy of the viewer viewport. It is not a
general copy of every temporary OpenGL viewport.

That means there are at least two different intents:

- set the canonical viewer viewport and update `gGLViewport`
- set a temporary render-target viewport without changing `gGLViewport`

`LLRenderTarget::flush()` depends on that distinction because it restores the
default framebuffer viewport from `gGLViewport`.

## Risk

Risk level: medium.

Reasons:

- `glViewport` is global OpenGL state.
- Temporary FBO viewports can leak into later passes if restore ordering is
  wrong.
- Updating `gGLViewport` for temporary viewports would be incorrect.
- Not updating `gGLViewport` for viewer-window viewports would break picking,
  projection, shader viewport uniforms, or later render target flush restore.
- Several callsites are in high-risk files: `pipeline.cpp`,
  `llviewerwindow.cpp`, and `llimagegl.cpp`.

## Containment Decision

Do not add a generic `LLGLContainment::viewport(...)` wrapper.

A useful future containment API would need to name the intent explicitly. Two
separate concepts are likely required:

- viewer viewport: updates `gGLViewport` and calls `glViewport`
- temporary render viewport: calls `glViewport` without changing `gGLViewport`

The first source-side candidate should be even narrower: document or contain
only `LLRenderTarget` viewport ownership before touching `pipeline.cpp` or UI
callers.

## Candidate First Source Patch

If a source patch is requested later, the smallest reasonable target is:

- file: `indra/llrender/llrendertarget.cpp`
- callsites:
  - `LLRenderTarget::bindTarget()`
  - `LLRenderTarget::flush()`
- intent:
  - bind target viewport
  - restore viewer viewport on return to the default framebuffer

Do not change `pipeline.cpp`, `llviewerwindow.cpp`, or probe managers in the
same patch.

## Verification Plan For A Future Patch

Build:

- compile `llrender/fast`
- run the local macOS arm64 Release viewer build if any non-`llrender` file is
  changed

Runtime smoke checks:

- login screen still appears
- resize the viewer window and verify the 2D UI and 3D world align
- run the empty-area FPS baseline scene
- run the loaded-area FPS baseline scene
- check a post-processing path such as DoF or antialiasing if `pipeline.cpp` is
  touched later
- check dynamic texture or preview rendering if UI preview paths are touched

## Next Step

Before any source patch, write a short contract for `LLRenderTarget` viewport
behavior:

- when it may change viewport
- when it must restore viewport
- whether it may update `gGLViewport`
- what caller ordering it assumes
