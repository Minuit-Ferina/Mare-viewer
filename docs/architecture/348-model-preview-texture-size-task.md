# Model Preview Texture Size Task

Date: 2026-05-24

Branch: `phase12`

## Task

Move model preview texture-size/render-target sizing ownership from
`LLFloaterModelPreview` to `LLModelPreview`.

## Current Behavior

`LLFloaterModelPreview::initModelPreview()` currently:

1. starts with `tex_width = 512` and `tex_height = 512`;
2. reads `gPipeline.mRT->width` and `gPipeline.mRT->height`;
3. clamps each render-target dimension to `PREVIEW_RENDER_SIZE`;
4. doubles each texture dimension while `(tex_dim << 1) < max_dim`;
5. constructs `LLModelPreview(tex_width, tex_height, this)`.

This makes the floater include `pipeline.h` and know about render-target
limits.

## Target Ownership

`LLModelPreview` should expose a small sizing method that returns the same
texture dimensions.

`LLFloaterModelPreview` should call that method and then construct the preview.

## Required Ordering

Preserve these constraints:

1. initial texture dimensions stay `512 x 512`;
2. maximum preview render size stays `1024`;
3. clamping against `gPipeline.mRT` dimensions stays unchanged;
4. power-of-two growth condition stays unchanged;
5. `LLModelPreview` construction order stays unchanged;
6. no preview panel layout, dynamic texture, model load, upload, LOD, physics,
   or render behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change texture dimensions;
- introduce fallback behavior for missing render targets;
- touch `pipeline.cpp`;
- touch broad `llui`;
- run a broad `mare-viewer` integration build for this packet.

## Verification

Run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
