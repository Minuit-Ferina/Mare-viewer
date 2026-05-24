# Model Preview Texture Quad Summary

Date: 2026-05-24

Branch: `phase11`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLModelPreview` now owns drawing its dynamic texture into the floater preview
rectangle through:

- `drawPreviewTexture(S32 left, S32 top, S32 right, S32 bottom)`

`LLFloaterModelPreview::draw3dPreview()` still owns:

- finding `preview_panel`;
- tracking `mPreviewRect`;
- refreshing the model preview when the rect changes.

It no longer owns the low-level texture bind, vertex emission, or texture
unbind for the preview quad.

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- preview panel rect lookup;
- rect-change refresh behavior;
- `mPreviewRect` coordinate use;
- texture coordinate order;
- vertex order;
- bind and unbind order;
- dynamic texture update order;
- model loading, upload, LOD, physics, and render behavior.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

The regenerated inventory reflects the expected ownership shift:

- `indra/newview/llfloatermodelpreview.cpp` line count decreased;
- `indra/newview/llmodelpreview.cpp` line count increased;
- OpenGL containment and header boundaries did not regress.

## Residual Risk

Remaining model preview UI/render coupling:

- render-time skin UI synchronization still happens through a floater-owned
  method;
- `LLModelPreview::render()` still receives UI-derived values every render;
- `LLFloaterModelPreview::draw3dPreview()` still decides when the preview
  texture is drawn based on floater state.

The next useful step is to map whether `draw3dPreview()` should become a pure
layout method that delegates all render work, or whether phase 11 should stop
after these ownership moves and run an integration checkpoint.
