# Image Preview Avatar Render Summary

Date: 2026-05-24

Branch: `phase8`

## Source Changes

Files changed:

- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llfloaterimagepreview.cpp`

`LLImagePreviewAvatar::render()` now delegates owner-local work to three private
helpers:

- `drawPreviewBackground()`
- `applyPreviewCamera(LLVOAvatar* avatarp)`
- `renderPreviewAvatar(LLVOAvatar* avatarp)`

The helper split keeps the existing order:

1. clear `mNeedsUpdate`;
2. push UI matrix and load UI identity;
3. push projection/modelview matrices;
4. create `LLGLSUIDefault`;
5. draw the dark preview background;
6. pop projection/modelview matrices;
7. flush `gGL`;
8. apply the preview camera;
9. unbind vertex buffers and update dummy avatar LOD;
10. render the dummy avatar when drawable/face data is available;
11. pop UI matrix;
12. restore immediate color to white;
13. return `true`.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not touch:

- image upload validation, cost, encoding, or permissions;
- preview type UI selection;
- dummy avatar target mesh setup;
- `LLImagePreviewSculpted`;
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

`LLImagePreviewAvatar` still mixes dummy avatar setup, camera state, UI preview
background drawing, and avatar draw-pool rendering in one owner. This packet
only names the existing render blocks so later work can decide whether the
adjacent sculpted preview should receive the same treatment.
