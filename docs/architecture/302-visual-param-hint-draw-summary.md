# Visual Param Hint Draw Summary

Date: 2026-05-23

Branch: `phase7`

## Source Changes

Files changed:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

`LLVisualParamHint::draw(F32 alpha)` now delegates to two private owner-local
helpers:

- `isHintVisibleForDraw() const`
- `drawHintTexture(F32 alpha)`

The helper split keeps the existing order:

1. return before any draw state mutation when the hint is not visible;
2. bind the dynamic texture on texture unit 0;
3. set immediate draw color to white with caller alpha;
4. draw the full hint quad under `LLGLSUIDefault`;
5. unbind texture unit 0.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet only names the UI-facing draw responsibilities after phase 6
finished the dynamic texture generation path.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Risk remains low.

The remaining `lltoolmorph` cleanup candidate is
`LLVisualParamReset::render()`, which owns avatar appearance reset after hint
rendering. That path should be mapped before any source changes.
