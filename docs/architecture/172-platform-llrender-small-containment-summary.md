# Platform And LLRender Small Containment Summary

Branch: `phase3`
Source commit: `ecb1666297`

## Scope Completed

Routed small remaining platform and `llrender` OpenGL calls through
`llglcontainment.*` where ownership and dependency direction allow it.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrender.cpp`
- `indra/llrender/llgl.h`
- `indra/llwindow/llwindowwin32.cpp`
- `indra/llwindow/llwindowsdl.cpp`
- `indra/llwindow/llwindowmesaheadless.cpp`

Added containment helpers:
- `setDebugMessageCallback`
- `hasVertexArrayGenerator`
- `finishCommands`

Reused containment helpers:
- `enableCapability`
- `setClearColor`
- `clearBuffers`
- `getInteger`
- `disableCapability`

Contained call families:
- `glDebugMessageCallback`
- `glEnable`
- `glGenVertexArrays` availability check
- `glClearColor`
- `glClear`
- `glGetIntegerv`
- `glDisable`
- `glFinish`

## Behavior Notes

Behavior is unchanged:
- Windows debug output setup still happens only under the existing
  `LL_WINDOWS` guard.
- Windows vertex-array generator availability is checked through the wrapper.
- Win32 startup clear still clears to transparent black.
- SDL graphics info still queries the same integer fields.
- Mesa headless swap still finishes pending GL work.

## Explicit Exceptions

Left outside wrapper containment:
- `indra/llwindow/llwindowsdl.cpp`: `glXGetProcAddressARB` remains platform
  loader glue.
- `indra/llwindow/llwindowmacosx-objc.h`: `glSwapBuffers` is a project bridge
  function name, not an OpenGL callsite.
- `indra/llcommon/llprofiler.h`: `LL_LABEL_OBJECT_GL` expands to RenderDoc GL
  labeling, but `llcommon` should not depend on `llrender`.
- `indra/llrender/llgl.h`: extension alias macros remain OpenGL compatibility
  definitions.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llwindow/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- `llwindow/fast` verifies the local Darwin window target only.
- SDL, Win32, and Mesa-headless platform files are off-platform for this local
  Darwin build and were not compiled here.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/llrender/llrender.cpp`: active direct `gl*` calls reduced from 3 to
  0.
- `indra/llwindow/llwindowwin32.cpp`: active direct `gl*` calls reduced from 2
  to 0.
- `indra/llwindow/llwindowmesaheadless.cpp`: active direct `gl*` calls reduced
  from 1 to 0.
- `indra/llwindow/llwindowsdl.cpp`: active direct `gl*` calls reduced from 11
  to 1, with the remaining call being GLX platform loader glue.
- `indra/llrender/llgl.h`: active direct `gl*` calls reduced from 1 to 0.

Runtime smoke:
- deferred unless requested.
