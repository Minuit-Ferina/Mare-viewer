# Image Preview Sculpted Render Summary

Date: 2026-05-24

Branch: `phase8`

## Source Changes

Files changed:

- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llfloaterimagepreview.cpp`

`LLImagePreviewSculpted::render()` now delegates owner-local work to three
private helpers:

- `drawPreviewBackground()`
- `applyPreviewCamera()`
- `renderSculptedVolume()`

The helper split keeps the existing order:

1. clear `mNeedsUpdate`;
2. create UI/default, no-blend, cull-face, and depth-test scopes;
3. push projection/modelview matrices;
4. draw the dark preview background;
5. pop projection/modelview matrices;
6. clear depth;
7. apply the preview camera;
8. bind preview shader and draw the sculpted volume;
9. unbind preview shader;
10. return `true`.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not touch:

- image upload validation, cost, encoding, or permissions;
- preview type UI selection;
- sculpt volume or vertex-buffer build logic;
- `LLImagePreviewAvatar`;
- dynamic texture driver behavior.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterimagepreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Risk remains medium.

`LLImagePreviewSculpted` still mixes preview camera state, background drawing,
shader state, lighting, and volume draw ownership in one class. This packet
only names the existing render blocks so later work can compare the avatar and
sculpt preview owners as a pair.
