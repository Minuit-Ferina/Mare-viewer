# LLSceneMonitor Containment Task

Branch: `phase3`
Source owner: `LLSceneMonitor`

## Scope

Route direct OpenGL calls in `indra/newview/llscenemonitor.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- `glBindFramebuffer`
- `glCopyTexSubImage2D`
- `glColorMask`
- `glBeginQuery`
- `glEndQuery`
- `glGetQueryObjectuiv`

Included behavior:
- scene-load frame capture from the main framebuffer
- copy into the capture texture
- hidden diff draw color-mask changes
- Windows-only sample-count query begin/end
- query availability and result reads

## Non-Scope

Do not change:
- scene loading monitor state transitions
- capture timing
- diff target dimensions
- query result interpretation
- debug-viewer visibility behavior
- Windows-only compile guards

Do not move scene monitoring into another owner.

## Ownership Notes

`LLSceneMonitor` keeps ownership of:
- capture cadence
- framebuffer restore target
- diff render sizing
- color-mask decision
- query lifetime and result interpretation

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: low to medium.

Why:
- This is wrapper-only, but the capture path touches framebuffer binding and
  texture copy ordering.
- Query rendering is Windows-only in this file.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llscenemonitor.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later with scene loading monitor enabled.
