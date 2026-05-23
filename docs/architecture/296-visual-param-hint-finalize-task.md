# Visual Param Hint Finalize Task

Date: 2026-05-23

Branch: `phase6`

## Scope

This task follows `docs/architecture/289-visual-param-hint-render-map.md` and
`docs/architecture/295-visual-param-hint-impostor-summary.md`.

The source packet may only touch:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Goal

Split visual-param restoration and hint texture finalization out of
`LLVisualParamHint::render()` into private owner-local helpers.

Allowed helpers:

- `restorePreviewVisualParamState()`
- `finalizeHintRender()`

## Behavior To Preserve

Do not change:

- visual-param restore after impostor generation;
- avatar visual-param restore before wearable visual-param restore;
- wearable volatile clear timing;
- `gAgentAvatarp->updateVisualParams()` timing;
- immediate color restore to white;
- `mGLTexturep->setGLTextureCreated(true)` timing;
- `gGL.popUIMatrix()` timing;
- return value.

## Risk

Risk is low to medium.

Why:

- this packet does not move camera or impostor generation;
- it only names the existing restore/finalize blocks;
- immediate color restore remains before texture-created marking.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
