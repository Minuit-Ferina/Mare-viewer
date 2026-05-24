# Model Preview Texture Quad Task

Date: 2026-05-24

Branch: `phase11`

## Task

Move the OpenGL textured-quad drawing for the model preview texture out of
`LLFloaterModelPreview::draw3dPreview()` and into `LLModelPreview`.

This is a UI/render boundary packet. It does not move source files.

## Current Behavior

`LLFloaterModelPreview::draw3dPreview()` currently:

1. sets draw color;
2. binds `mModelPreview` as a texture;
3. looks up `preview_panel`;
4. refreshes the model preview if the panel rect changed;
5. draws two triangles over `mPreviewRect`;
6. unbinds the texture.

The floater therefore owns both UI layout and low-level draw calls.

## Target Ownership

`LLFloaterModelPreview` should keep UI/layout ownership:

- locate `preview_panel`;
- track `mPreviewRect`;
- request a refresh when the rect changes.

`LLModelPreview` should own drawing its preview texture:

- bind the dynamic texture;
- emit the textured quad vertices;
- unbind the texture.

## Required Ordering

Preserve these constraints:

1. The preview panel rect is read before drawing.
2. Rect-change refresh behavior remains in `LLFloaterModelPreview`.
3. The same `mPreviewRect` coordinates are used for the quad.
4. Texture bind, vertex order, texture coordinates, and unbind order are
   unchanged.
5. No dynamic texture update, model render, model loading, upload, LOD, or
   physics behavior changes.

## Explicit Non-Goals

Do not:

- move source files;
- change preview panel layout handling;
- change dynamic texture ordering;
- change OpenGL containment APIs;
- touch `pipeline.cpp`;
- touch broad `llui`;
- change `needsRender()`.

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
