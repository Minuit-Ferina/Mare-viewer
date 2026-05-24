# Model Preview Skinned Summary

Date: 2026-05-24

Branch: `phase10`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

`LLModelPreview::render()` now delegates skinned avatar preview drawing to:

- `renderSkinnedPreview(...)`

The helper keeps the existing order:

1. the caller remains inside the `show_skin_weight` branch;
2. preview avatar camera target recentering stays before skinned draw work;
3. attachment overrides are cleared before pelvis fixup is added;
4. joint override setup stays before matrix palette upload and draw;
5. matrix palette upload remains immediately before each skinned buffer draw;
6. material application and texture unbind order are unchanged;
7. optional edge overlay state order is unchanged;
8. collision-volume, bone, and ground-plane debug draw order is unchanged;
9. previous shader rebinding is preserved after debug draw;
10. pelvis recalculation remains after skinned and debug drawing.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not touch:

- joint override semantics;
- pelvis fixup behavior;
- matrix palette upload order;
- skinned draw order;
- debug bone or collision-volume display;
- material binding semantics;
- model, physics, upload, or validation behavior;
- dynamic texture order or target behavior.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

`LLModelPreview::render()` is now more structured, but still owns too much:

- UI mutation remains in the render path;
- buffer generation can still happen during render;
- all draw helpers still live in the same owner class;
- the model upload floater and preview renderer remain tightly coupled.

The next useful step is not another tiny helper. It should be a decision point:

- either close phase 10 with a non-clean integration build checkpoint; or
- map a real behavior-preserving separation of UI mutation from render work
  before moving code.
