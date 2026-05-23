# Visual Param Reset Summary

Date: 2026-05-23

Branch: `phase7`

## Source Changes

Files changed:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

`LLVisualParamReset::render()` now delegates avatar appearance reset work to one
private owner-local helper:

- `resetAvatarAppearanceState()`

The helper split keeps the existing order:

1. check `sDirty`;
2. update avatar composites;
3. update avatar visual params;
4. update avatar geometry with `gAgentAvatarp->mDrawable`;
5. clear `sDirty`;
6. return `false`.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not add a drawable null guard, does not change
`ORDER_RESET`, and does not change when `LLVisualParamHint` marks reset state
dirty.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Risk remains low to medium.

`LLVisualParamReset::render()` still depends on dynamic texture order and
`LLVisualParamHint::render()` setting `sDirty`. That coupling is intentional
and remains documented rather than changed in phase 7.
