# Platform OpenGL Boundaries

This document maps platform-specific OpenGL context and swap boundaries. It is a
phase 1 analysis artifact only. It does not propose source edits.

Source inputs:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/04-gl-callsite-inventory.md`
- `docs/architecture/06-shader-map.md`
- targeted reads of the platform and FSR2 files listed below

## Files Inspected

- `indra/llwindow/CMakeLists.txt`
- `indra/llwindow/llwindowmacosx.cpp`
- `indra/llwindow/llwindowmacosx.h`
- `indra/llwindow/llwindowmacosx-objc.h`
- `indra/llwindow/llwindowmacosx-objc.mm`
- `indra/llwindow/llopenglview-objc.h`
- `indra/llwindow/llopenglview-objc.mm`
- `indra/llwindow/llwindowwin32.cpp`
- `indra/llwindow/llwindowwin32.h`
- `indra/llwindow/llwindowsdl.cpp`
- `indra/llwindow/llwindowmesaheadless.cpp`
- `indra/newview/CMakeLists.txt`
- `indra/newview/marefsr2upscaler.h`
- `indra/newview/marefsr2upscaler.cpp`
- `indra/newview/maretaaupscaler.cpp`
- `indra/newview/pipeline.cpp`
- `indra/newview/app_settings/shaders/class1/deferred/fsr2/`

## Inventory Snapshot

| file | category in generated inventory | gl_calls | gGL | LLGL | LLRenderTarget | platform role |
|---|---|---:|---:|---:|---:|---|
| `indra/llwindow/llwindowsdl.cpp` | `assets.texture` | 12 | 0 | 0 | 0 | Linux/SDL OpenGL window and GLX capability queries |
| `indra/llwindow/llwindowwin32.cpp` | `render.opengl_touching` | 4 | 0 | 0 | 0 | Windows WGL context creation and startup clear |
| `indra/llwindow/llwindowmacosx-objc.h` | `render.opengl_touching` | 3 | 0 | 0 | 0 | Darwin OpenGL bridge declarations, including local `glSwapBuffers` helper |
| `indra/llwindow/llwindowmacosx.cpp` | `unknown` | 0 | 0 | 0 | 0 | Darwin CGL/NSOpenGL context owner |
| `indra/llwindow/llwindowmesaheadless.cpp` | `render.opengl_touching` | 1 | 0 | 0 | 0 | OSMesa headless context and swap flush |
| `indra/newview/marefsr2upscaler.cpp` | `render.opengl_touching` | 81 | 3 | 3 | 4 | optional OpenGL compute upscaler backend |
| `indra/newview/marefsr2upscaler.h` | `render.opengl_touching` | 2 | 0 | 1 | 5 | FSR2 interface comments and render target references |

Inventory caveat: the generated `gl_calls` count is regex-based. In platform
files it can include local helper names such as `glSwapBuffers` and `glReady`,
not only Khronos OpenGL API calls.

## Platform Entry Points

| platform | primary files | context path | swap path | risk |
|---|---|---|---|---|
| Darwin | `llwindowmacosx.cpp`, `llwindowmacosx-objc.mm`, `llopenglview-objc.mm` | Cocoa `NSOpenGLView` and CGL pixel format creation. | local `glSwapBuffers(void*)` helper calls `NSOpenGLContext::flushBuffer`. | High |
| Windows | `llwindowwin32.cpp` | WGL pixel format selection, WGL context creation, version fallback, optional core/compat profile. | `SwapBuffers(mhDC)`. | High |
| Linux SDL/X11 | `llwindowsdl.cpp` | SDL OpenGL window creation, SDL GL attributes, GLX extension queries. | `SDL_GL_SwapBuffers()`. | Medium-high |
| Mesa/headless | `llwindowmesaheadless.cpp` | `OSMesaCreateContextExt`, image buffer allocation, `OSMesaMakeCurrent`. | `glFinish()`. | Medium |

## Darwin OpenGL Boundary

Darwin uses the Objective-C bridge in `llwindowmacosx-objc.mm` and
`llopenglview-objc.mm` to create an `NSOpenGLView` and `NSOpenGLContext`.

Observed context attributes:

- `NSOpenGLPFAOpenGLProfile`
- `NSOpenGLProfileVersion4_1Core`
- accelerated double-buffered pixel format
- depth 24, stencil 8, alpha 8, color 24
- optional multisampling through the view constructor

`llwindowmacosx.cpp` also builds a CGL pixel format with
`kCGLPFAOpenGLProfile` and `kCGLOGLPVersion_GL4_Core`.

The local helper named `glSwapBuffers(void* context)` is declared in
`llwindowmacosx-objc.h` and implemented in `llwindowmacosx-objc.mm`. It calls
`flushBuffer` on the `NSOpenGLContext`. Treat this as platform glue, not a
renderer-layer OpenGL API call.

Risk: high. This is the viewer's Darwin context boundary. It should not be
changed during early phase 1 except for narrowly scoped build fixes.

## Darwin FSR2 Exclusion Rationale

`indra/newview/CMakeLists.txt` currently defines:

- `MARE_ENABLE_FSR2` as an option defaulting to `ON`.
- Darwin override: when `DARWIN` and `MARE_ENABLE_FSR2` are both true, CMake
  logs that FSR2 is disabled because macOS OpenGL lacks the required compute API,
  then sets `MARE_ENABLE_FSR2` to `OFF`.
- `marefsr2upscaler.cpp` is appended to `viewer_SOURCE_FILES` only when
  `MARE_ENABLE_FSR2` is true.
- The viewer target receives `MARE_ENABLE_FSR2=1` or `MARE_ENABLE_FSR2=0`.

This is the correct build containment for Darwin.

Reason: the MARE FSR2 backend is not just normal fragment-shader rendering. It
uses raw OpenGL compute and image-load/store style APIs, including:

- `GL_COMPUTE_SHADER`
- `glDispatchCompute`
- `glMemoryBarrier`
- `glBindImageTexture`
- `glCreateTextures`
- `glTextureStorage2D`
- `glTextureParameteri`

The shader assets under `app_settings/shaders/class1/deferred/fsr2/` are
`.comp.glsl` compute shaders with `layout(local_size_x = 8, local_size_y = 8)`
and image writes through `imageStore`.

macOS OpenGL is capped at the 4.1 core profile in this code path, while this
FSR2 implementation requires newer compute/image/DSA functionality. Therefore
the Darwin exclusion is platform-capability containment, not an AMD-vs-non-AMD
GPU decision.

Runtime impact of the build flag:

- `marefsr2upscaler.cpp` is not compiled on Darwin.
- `IUpscaler::create()` does not expose the FSR2 case when
  `MARE_ENABLE_FSR2=0`.
- `pipeline.cpp` gates FSR2 render-resolution and display-target behavior with
  `MARE_ENABLE_FSR2`.
- TAA and NIS paths remain available because they use the existing
  `LLGLSLShader` fragment-shader flow and `LLRenderTarget`.

## Windows OpenGL Boundary

`llwindowwin32.cpp` owns WGL setup:

- `ChoosePixelFormat` and `SetPixelFormat`
- initial legacy context creation through `wglCreateContext`
- `gGLManager.initWGL()`
- ARB pixel format selection when available
- shared context creation through `wglCreateContextAttribsARB`
- fallback from higher requested versions down toward OpenGL 3.0
- profile selection from `LLRender::sGLCoreProfile`
- debug context flag from `gDebugGL`
- swap interval through `wglSwapIntervalEXT`
- frame presentation through `SwapBuffers(mhDC)`

The refined inventory's `glReady` symbol is a local `WindowThread` method name,
not an OpenGL API call. The direct startup clear near window creation uses
`glClearColor`, `glClear`, then `swapBuffers()` when `auto_show` is true.

Risk: high. This file is platform glue plus startup rendering state. Keep it
separate from renderer containment work unless the task is explicitly about
Windows context creation.

## Linux SDL/X11 OpenGL Boundary

`llwindowsdl.cpp` owns the SDL path:

- SDL GL attributes for color, alpha, depth, stencil, double buffering, and
  multisampling.
- window/context creation through `SDL_SetVideoMode(... SDL_OPENGL ...)`.
- `gGLManager.initGL()` after the context exists.
- GLX extension lookup through `glXGetProcAddressARB`.
- VRAM probing through MESA, NVX, and ATI memory queries.
- buffer bit queries through `glGetIntegerv`.
- default multisampling disable through `glDisable(GL_MULTISAMPLE_ARB)`.
- frame presentation through `SDL_GL_SwapBuffers()`.

Risk: medium-high. The file mixes old SDL windowing, X11/GLX probing, and
OpenGL capability queries. It should be documented as platform/context glue, not
renderer pass logic.

## Mesa Headless Boundary

`llwindowmesaheadless.cpp` owns the headless OSMesa path:

- creates an offscreen context with `OSMesaCreateContextExt`
- allocates a backing image buffer
- binds it with `OSMesaMakeCurrent`
- calls `gGLManager.initGL()`
- presents by calling `glFinish()` in `swapBuffers()`

Risk: medium. It is a small file, but it is the non-windowed GL context path.
Do not conflate it with the viewer runtime window implementations.

## Containment Notes

- Treat `indra/llwindow/` as platform/context glue, not as the renderer
  abstraction layer.
- Keep Darwin's `MARE_ENABLE_FSR2=0` guard until a non-compute fallback is
  explicitly designed and tested.
- Do not move platform files during phase 1.
- Do not use platform OpenGL files as the first place to introduce renderer
  abstractions. Their contracts are OS-specific.
- Keep raw inventory false positives visible until the generator reports helper
  names separately from real OpenGL calls.

## Areas Not To Modify First

- Darwin context/profile creation in `llopenglview-objc.mm` and
  `llwindowmacosx.cpp`
- Windows WGL version fallback in `LLWindowWin32::createSharedContext()`
- SDL GL attribute and context setup in `LLWindowSDL::createContext()`
- OSMesa context creation in `LLWindowMesaHeadless`
- FSR2 compute backend source inclusion and compile definition gates

Reason: these areas decide whether a GL context exists at all, which GL version
is available, and which optional backend code can compile on each platform.

## Small Follow-Up Tasks

- Refine `tools/architecture/source_inventory.py` to split raw `gl*` references
  from likely API calls and known helper-name false positives.
- Add a short review checklist for new platform OpenGL code.
- Keep local Darwin arm64 build notes separate from release/universal packaging
  decisions.
- Capture runtime FPS baseline before proposing platform or renderer behavior
  changes.
