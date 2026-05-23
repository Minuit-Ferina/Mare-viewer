# Visual Param Hint Finalize Summary

Date: 2026-05-23

Branch: `phase6`

## Source Changes

Files changed:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

`LLVisualParamHint::render()` now delegates its final restore/finalize blocks to
two private owner-local helpers:

- `restorePreviewVisualParamState()`
- `finalizeHintRender()`

The helper split keeps the existing order:

1. render avatar impostor;
2. restore avatar and wearable visual param weights;
3. clear wearable volatility;
4. update avatar visual params;
5. restore draw color to white;
6. mark the dynamic texture as created;
7. pop the UI matrix;
8. return `true`.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet only names the existing finalization responsibilities so later work
can reason about:

- preview state restore;
- wearable volatility lifetime;
- final dynamic texture state;
- remaining UI matrix ownership in `LLVisualParamHint::render()`.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Risk remains low.

The main residual coupling is that `LLVisualParamHint::render()` still owns the
outer UI matrix lifetime and still coordinates all helper ordering. That is
intentional for this phase because the phase 6 work is limited to small,
owner-local separation around appearance-editor preview rendering.
