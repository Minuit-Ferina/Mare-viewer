# Visual Param Hint Draw Task

Date: 2026-05-23

Branch: `phase7`

## Scope

This task follows `docs/architecture/300-visual-param-hint-draw-map.md`.

The source packet may only touch:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Goal

Split `LLVisualParamHint::draw(F32 alpha)` into private owner-local helpers
without changing draw behavior.

Allowed helpers:

- `isHintVisibleForDraw() const`
- `drawHintTexture(F32 alpha)`

## Behavior To Preserve

Do not change:

- early return when `mIsVisible` is false;
- texture unit 0 binding target;
- immediate color `(1.f, 1.f, 1.f, alpha)`;
- `LLGLSUIDefault` scope;
- primitive type `LLRender::TRIANGLES`;
- vertex order;
- texture coordinates;
- texture unbind after `gGL.end()`.

## Risk

Risk is low.

Why:

- the packet only names the visibility gate and textured quad draw;
- it does not touch dynamic texture generation;
- it does not touch avatar state, camera state, or reset ordering.

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
