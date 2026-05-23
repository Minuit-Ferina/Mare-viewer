# Visual Param Hint Background Task

Date: 2026-05-23

Branch: `phase6`

## Scope

This task follows `docs/architecture/289-visual-param-hint-render-map.md` and
`docs/architecture/291-visual-param-hint-camera-summary.md`.

The source packet may only touch:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Goal

Split the hint background projection/modelview draw block out of
`LLVisualParamHint::render()` into a private owner-local helper.

Allowed helper:

- `drawHintBackground()`

## Behavior To Preserve

Do not change:

- `gGL.pushUIMatrix()` / `gGL.popUIMatrix()` lifetime;
- UI identity load timing;
- projection matrix push/pop ordering;
- modelview matrix push/pop ordering;
- ortho projection dimensions;
- `gUIProgram.bind()` timing;
- `LLGLSUIDefault` scope around background draw;
- background draw coordinates and dimensions;
- update/visible flag timing;
- camera setup;
- impostor generation;
- visual-param restore ordering.

## Risk

Risk is medium.

Why:

- this packet moves render-state calls into a helper;
- the matrix push/pop order must remain identical;
- the outer UI matrix lifetime remains intentionally unchanged.

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
