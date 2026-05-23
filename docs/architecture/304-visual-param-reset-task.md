# Visual Param Reset Task

Date: 2026-05-23

Branch: `phase7`

## Scope

This task follows `docs/architecture/303-visual-param-reset-map.md`.

The source packet may only touch:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Goal

Split `LLVisualParamReset::render()` avatar reset work into a private
owner-local helper without changing reset behavior.

Allowed helper:

- `resetAvatarAppearanceState()`

## Behavior To Preserve

Do not change:

- `sDirty` as the only reset gate;
- `sDirty = false` timing after reset work;
- composite update before visual param update;
- visual param update before geometry update;
- geometry update argument `gAgentAvatarp->mDrawable`;
- return value `false`;
- `LLViewerDynamicTexture::ORDER_RESET`.

## Risk

Risk is low to medium.

Why:

- this packet only names the existing reset block;
- it does not change dynamic texture ordering;
- it does not add new guards or fallback behavior;
- avatar reset timing remains coupled to the hint render dirty flag.

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
