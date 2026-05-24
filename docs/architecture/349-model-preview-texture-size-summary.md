# Model Preview Texture Size Summary

Date: 2026-05-24

Branch: `phase12`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

`LLModelPreview` now owns model preview dynamic texture sizing through:

- `getPreviewTextureSize(S32& tex_width, S32& tex_height)`

`LLFloaterModelPreview::initModelPreview()` now asks `LLModelPreview` for the
texture size before constructing the preview.

## Behavior Notes

No runtime behavior is intentionally changed.

The packet preserves:

- initial texture dimensions of `512 x 512`;
- maximum preview render size of `1024`;
- clamping against `gPipeline.mRT->width` and `gPipeline.mRT->height`;
- the power-of-two growth condition;
- `LLModelPreview` construction order;
- model loading, upload, LOD, physics, preview panel layout, and dynamic
  texture behavior.

`LLFloaterModelPreview` no longer includes `pipeline.h` and no longer reads
`gPipeline.mRT` directly.

The packet also added an explicit `llappviewer.h` include because the removed
`pipeline.h` include had been providing `LLAppViewer` indirectly.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

No broad `mare-viewer` integration build was run.

## Residual Risk

Remaining model preview coupling:

- render-time skin UI synchronization still happens through a floater-owned
  method;
- `LLModelPreview::render()` still receives UI-derived values every render;
- broader model preview load/update paths still mutate floater controls;
- `LLFloaterModelPreview::draw3dPreview()` still owns preview panel layout and
  decides when to draw the preview texture.

The next useful packet should continue removing concrete coupling, preferably
one of the remaining non-render control reads or status/control mutations in
`LLModelPreview`, with targeted object builds only.
