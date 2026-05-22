# Profiler SDL MacOSX Edge Containment Task

Branch: `phase3`
Source owners:
- `llcommon` profiler macros
- SDL windowing
- macOS Objective-C bridge

## Scope

Close the three requested small edge cases from the generated OpenGL inventory:
- `indra/llcommon/llprofiler.h`
- `indra/llwindow/llwindowsdl.cpp`
- `indra/llwindow/llwindowmacosx-objc.h`

Included behavior:
- RenderDoc object-label macro expansion
- SDL GLX extension-function lookup
- macOS bridge function naming for buffer flushing

## Non-Scope

Do not change:
- RenderDoc labeling behavior
- SDL VRAM probing behavior
- GLX extension lookup target
- macOS context flushing behavior
- `llcommon` dependency direction

Do not add a dependency from `llcommon` to `llrender`.

## Ownership Notes

`llcommon/llprofiler.h` keeps ownership of profiler macros.

`llwindowsdl.cpp` keeps ownership of platform loader glue.

`llwindowmacosx-objc.*` keeps ownership of the Objective-C bridge.

`llglcontainment.*` is not used for `llcommon`, because `llcommon` is below
`llrender`.

## Risk

Risk: low.

Why:
- The changes are naming/macro-indirection only.
- No runtime order, queried value, or GLX lookup target changes.

## Verification

Required:
- `git diff --check`
- `llcommon/fast`
- `llwindow/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested.
