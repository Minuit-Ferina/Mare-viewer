# Visual Param Hint Camera Task

Date: 2026-05-23

Branch: `phase6`

## Scope

This task follows `docs/architecture/289-visual-param-hint-render-map.md`.

The source packet may only touch:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Goal

Split pure camera math out of `LLVisualParamHint::render()` into private
owner-local helpers without changing behavior.

Allowed helpers:

- `getAvatarRenderRotation() const`
- `getCameraTargetPosition(const LLQuaternion& avatar_rotation,
  const LLVector3& target_joint_pos) const`
- `getCameraPosition(const LLQuaternion& avatar_rotation,
  const LLVector3& target_joint_pos) const`

## Behavior To Preserve

Do not change:

- avatar root rotation fallback;
- target joint world-position read timing;
- visual-param camera elevation use;
- visual-param camera angle use;
- `AppearanceCameraMovement` policy;
- `F_PI` angle adjustment when automatic camera movement is disabled;
- visual-param camera distance use;
- camera aspect/perspective setup;
- matrix push/pop ordering;
- background draw timing;
- impostor generation;
- visual-param restore ordering.

## Risk

Risk is low to medium.

Why:

- the packet does not move `gGL` calls;
- the packet does not move camera application calls;
- it only names the existing camera math inputs and formulas.

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
