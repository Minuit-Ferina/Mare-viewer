# LLVOAvatar Containment Task

Branch: `phase3`
Source owner: `LLVOAvatar`

## Scope

Route direct OpenGL calls in `indra/newview/llvoavatar.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- `glLineWidth`
- `glGenQueries`
- `glBeginQuery`
- `glEndQuery`
- `glGetQueryObjectui64v`

Included behavior:
- impostor debug outline line width
- avatar GPU profile query allocation
- avatar GPU profile query begin/end
- avatar GPU profile query availability/result readback

## Non-Scope

Do not change:
- avatar render order
- impostor generation or sampling
- debug outline geometry
- GPU profile retry behavior
- query lifetime beyond the existing allocation path
- debug text formatting

Do not move avatar render profiling into another owner.

## Ownership Notes

`LLVOAvatar` keeps ownership of:
- avatar impostor debug rendering decisions
- GPU profile query state
- retry scheduling
- render-time conversion and debug text

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: low.

Why:
- This is wrapper-only.
- The active call families already have containment helpers.
- The affected query path is narrow and keeps the same query target and result
  variables.

## Verification

Required:
- `git diff --check`
- targeted `llvoavatar.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; later checks should include an avatar-heavy scene
  and impostor debug/profile UI if those modes are used.
