# Visual Param Hint Needs Render Task

Date: 2026-05-23

Branch: `phase6`

## Scope

This task follows `docs/architecture/284-visual-param-hint-owner-map.md`.

The source packet may only touch:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Goal

Split `LLVisualParamHint::needsRender()` into private owner-local predicate
helpers without changing behavior.

Allowed helpers:

- `hasPendingUpdate() const`
- `isUpdateDelayElapsed()`
- `isAppearanceAnimationBlocked() const`
- `canRenderHint() const`

## Behavior To Preserve

Do not change:

- short-circuit order;
- `mNeedsUpdate` gate;
- `mDelayFrames-- <= 0` post-decrement behavior;
- appearance-animation gate;
- `mAllowsUpdates` gate;
- `requestHintUpdates(...)`;
- `preRender(...)`;
- `render()`;
- `draw(...)`;
- `LLVisualParamReset::render()`.

## Risk

Risk is low to medium.

Why:

- the packet does not touch render state;
- the packet does not touch avatar visual params;
- the only subtle behavior is preserving the delayed-update post-decrement
  exactly.

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
