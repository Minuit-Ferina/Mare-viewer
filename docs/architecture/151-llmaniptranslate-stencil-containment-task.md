# LLManipTranslate Stencil Containment Task

Branch: `phase3`
Source owner: `LLManipTranslate`

## Scope

Route direct OpenGL calls in the deprecated translation-intersection path of
`indra/newview/llmaniptranslate.cpp` through `llglcontainment.*`.

Included raw OpenGL families:
- `glCullFace`
- `glStencilOp`
- `glStencilFunc`
- `glStencilMask`

Included behavior:
- disabled grid cross-section cull face selection
- disabled stencil operation setup
- disabled stencil function and mask restore

## Non-Scope

Do not change:
- manipulator rendering
- active translation behavior
- grid rendering
- selection center math
- the `#if 0` disabled state of the deprecated path

Do not revive or remove the deprecated path.

## Ownership Notes

`LLManipTranslate` keeps ownership of:
- manipulator mode behavior
- deprecated cross-section render ordering
- selected stencil values
- cull direction selection

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: low.

Why:
- This is wrapper-only and the affected path is currently disabled by `#if 0`.
- The change still matters for inventory cleanliness and future reactivation
  review.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llmaniptranslate.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred; the edited path is disabled.
