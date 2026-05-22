# LLModelPreview Debug Containment Task

Branch: `phase3`
Source owner: `LLModelPreview`

## Scope

Route direct OpenGL calls in `indra/newview/llmodelpreview.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- `glLineWidth`
- `glPolygonMode`
- `glPointSize`
- `glClear`

Included behavior:
- preview mesh edge overlay
- physics mesh edge overlay
- degenerate triangle/point preview overlay
- preview depth buffer clear before physics rendering

## Non-Scope

Do not change:
- model import state
- preview camera setup
- mesh upload data ownership
- vertex buffer draw order
- material/texture binding order
- preview shader selection
- preview UI control behavior

Do not move model preview rendering into another owner.

## Ownership Notes

`LLModelPreview` keeps ownership of:
- preview mode selection
- mesh/physics/degenerate overlay ordering
- chosen line and point widths
- depth clear placement
- vertex buffer draw calls

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers
- fixed-function API names

## Risk

Risk: low to medium.

Why:
- This is wrapper-only and limited to model preview rendering.
- The file mixes UI-owned preview state with rendering, so the patch must keep
  draw order and state restoration unchanged.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llmodelpreview.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later by opening model upload/preview UI.
