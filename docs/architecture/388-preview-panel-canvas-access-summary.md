# Preview Panel Canvas Access Summary

Date: 2026-05-24

Branch: `phase13`

## Summary

This packet moved a first group of preview panel, canvas, and draw-rectangle
lookups behind owner-local helper methods.

The intent is breadth-first containment of preview/UI-render coupling, not a
runtime rendering change.

## Files Changed

- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llpreviewtexture.cpp`
- `indra/newview/llpreviewtexture.h`
- `indra/newview/lltexturectrl.cpp`
- `indra/newview/lltexturectrl.h`
- `indra/newview/llfloatermodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

## Ownership Changes

- `LLFloaterImagePreview` now owns its preview draw-right calculation.
- `LLPreviewTexture` now owns its texture preview border/interior rectangles.
- `LLFloaterTexturePicker` now owns its preview-widget border/interior
  rectangles.
- `LLFloaterModelPreview` now owns lookup of the model preview panel rectangle.
- `LLModelPreview` still owns camera setup, but no longer reaches directly
  through the floater child hierarchy for the preview panel rectangle.

## Behavior Notes

No intended behavior change:

- draw call order is unchanged;
- texture preview border math is unchanged;
- texture picker preview widget math is unchanged;
- model preview camera aspect uses the same preview panel rectangle;
- no OpenGL callsite was added, removed, or reordered.

## Verification

Targeted object builds passed:

- `llfloaterimagepreview.cpp.o`
- `llpreviewtexture.cpp.o`
- `lltexturectrl.cpp.o`
- `llmodelpreview.cpp.o`
- `llfloatermodelpreview.cpp.o`

Architecture checks passed:

- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

The regenerated inventory scanned 3085 source files.

## Risk

Risk is low. The packet only centralizes existing rectangle calculations behind
the classes that already own the associated UI elements.

The main residual risk is a missed implicit assumption around widget lifetime,
but the helper calls execute at the same points where the direct lookups already
executed.

