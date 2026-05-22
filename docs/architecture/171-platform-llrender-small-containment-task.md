# Platform And LLRender Small Containment Task

Branch: `phase3`
Source owners: platform windowing and small `llrender`

## Scope

Route small remaining non-pipeline OpenGL calls through `llglcontainment.*`
where the target already depends on `llrender`, and clean comment-only false
positives where wrapping is not applicable.

Included source files:
- `indra/llrender/llrender.cpp`
- `indra/llrender/llgl.h`
- `indra/llwindow/llwindowwin32.cpp`
- `indra/llwindow/llwindowsdl.cpp`
- `indra/llwindow/llwindowmesaheadless.cpp`

Included call families:
- debug message callback setup
- debug-output synchronous enable
- vertex array generator availability check
- startup clears in Win32 windowing
- SDL/X11 graphics info integer queries
- SDL multisample disable
- Mesa headless finish

## Non-Scope

Do not change:
- `pipeline.cpp`
- FSR2 backend
- `llgl.cpp`
- `llglheaders.h`
- Objective-C macOS bridge function names
- `llcommon/llprofiler.h` RenderDoc macro dependency

Do not move platform windowing ownership into `llrender`.

## Ownership Notes

Platform files keep ownership of:
- platform context creation
- graphics info probing
- swap behavior
- platform startup clear behavior

`LLRender` keeps ownership of:
- renderer initialization
- debug-output policy
- core-profile vertex-array availability checks

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: medium.

Why:
- This is wrapper-only for active behavior.
- Some source files are off-platform for the local Darwin build and cannot be
  compiled locally in this environment.
- `glXGetProcAddressARB` is left as platform loader glue rather than wrapped
  through the OpenGL containment API.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- local `llwindow/fast`
- targeted or platform-available object build where possible
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested.
