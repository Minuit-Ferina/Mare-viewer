# Visual Param Hint PreRender Task

Date: 2026-05-23

Branch: `phase6`

## Scope

This task follows `docs/architecture/284-visual-param-hint-owner-map.md` and
`docs/architecture/286-visual-param-hint-needs-render-summary.md`.

The source packet may only touch:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Goal

Split `LLVisualParamHint::preRender(...)` avatar-state setup into private
owner-local helpers without changing behavior.

Allowed helpers:

- `setWearableVolatile(bool is_volatile)`
- `applyPreviewVisualParamWeight()`
- `updatePreviewAvatarGeometry()`

## Behavior To Preserve

Do not change:

- wearable volatile timing;
- `mLastParamWeight` capture timing;
- preview visual param weight applied to wearable and agent avatar;
- blink suppression;
- composite update timing;
- `LLCharacter::updateVisualParams()` use;
- avatar geometry and LOD update timing;
- warning behavior when the avatar drawable is missing;
- `LLViewerDynamicTexture::preRender(clear_depth)` call order;
- `render()`, `draw(...)`, or `LLVisualParamReset::render()`.

## Risk

Risk is medium.

Why:

- this packet does not touch render-state calls directly;
- it does touch avatar preview state mutation order, which must remain exact.

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
