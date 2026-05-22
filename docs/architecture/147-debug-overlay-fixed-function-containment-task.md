# Debug Overlay Fixed-Function Containment Task

Branch: `phase3`
Source owners:
- `LLViewerObjectList` / `LLSky` debug beacon rendering
- `LLSelectMgr` selected-object wireframe rendering

## Scope

Route small debug-overlay fixed-function OpenGL calls through
`llglcontainment.*`.

Included source files:
- `indra/newview/llglsandbox.cpp`
- `indra/newview/llselectmgr.cpp`

Included raw OpenGL families:
- `glLineWidth`
- `glPolygonMode`

Included behavior:
- debug beacon line width changes
- sun/moon beacon line width changes
- selected-object wireframe polygon mode restore

## Non-Scope

Do not change:
- beacon geometry
- beacon color or text behavior
- selected-object iteration
- wireframe selection shader behavior
- hidden-selection behavior

Do not move debug rendering into another owner.

## Ownership Notes

The viewer/debug owners keep ownership of:
- overlay branch selection
- chosen line widths
- selected-object wireframe ordering
- shader rebinding

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: low.

Why:
- This is wrapper-only and uses existing containment helpers.
- The touched paths are debug/selection overlays, but selection visuals are
  user-facing during editing.

## Verification

Required:
- `git diff --check`
- targeted `llglsandbox.cpp.o` build
- targeted `llselectmgr.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later by enabling debug beacons and object
  selection wireframe.
