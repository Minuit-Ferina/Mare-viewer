# LLFace Debug Containment Task

Branch: `phase3`
Source owner: `LLFace`

## Scope

Route direct OpenGL calls in `indra/newview/llface.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- `glPolygonOffset`
- `glVertexPointer`
- `glEnableClientState`
- `glTexCoordPointer`
- `glDrawElements`
- `glDisableClientState`
- `glLineWidth`
- `glPolygonMode`

Included behavior:
- disabled rigged face selection debug draw path
- selected face wireframe overlay

## Non-Scope

Do not change:
- face selection behavior
- vertex buffer ownership
- rigged face TODO state
- region/object transform selection
- selected-face color or blend behavior
- draw order

Do not move `LLFace` rendering into another owner.

## Ownership Notes

`LLFace` keeps ownership of:
- selected face render order
- wireframe overlay state
- rigged debug draw data source if the disabled path is ever re-enabled
- transform selection
- color and blend decisions

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: low to medium.

Why:
- This is wrapper-only.
- The active path is a selected-face wireframe overlay; the rigged direct draw
  block remains disabled by `#if 0`.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llface.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; later checks should select/edit a face and verify
  wireframe selection overlay.
