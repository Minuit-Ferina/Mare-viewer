# Model Preview Canvas Summary

Date: 2026-05-24

Branch: `phase10`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

`LLModelPreview::render()` now delegates the preview canvas background draw to
a private owner-local helper:

- `drawPreviewCanvas(S32 width, S32 height)`

The helper keeps the existing order:

1. `LLModelPreview::render()` locks the preview and clears `mNeedsUpdate`;
2. view options and preview size are read;
3. existing UI/default, no-blend, cull, and disabled-depth scopes are created;
4. `drawPreviewCanvas(...)` binds `gUIProgram`;
5. projection/modelview matrices are pushed and loaded;
6. the orthographic UI projection is applied;
7. `PREVIEW_CANVAS_COL` is drawn with `gl_rect_2d_simple(...)`;
8. projection/modelview matrices are popped;
9. `gUIProgram` is unbound;
10. all existing model, UI, physics, avatar, and shader paths continue in the
    same order.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not touch:

- model parsing, upload, or validation;
- LOD generation;
- vertex buffer generation;
- model, material, physics, skinned avatar, joint, or debug drawing;
- UI mutation inside `LLModelPreview::render()`;
- camera math;
- shader choices;
- dynamic texture order or target behavior;
- `pipeline.cpp` or broad `llui`.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

Risk remains high for the rest of `LLModelPreview::render()`.

The method still mixes UI mutation, camera setup, buffer generation, ordinary
model draw, textured material preview, edge overlay, physics hull/mesh preview,
degenerate triangle debug drawing, skinned avatar preview, joint override
preview, and debug bone/ground-plane drawing.

The next source packet should stay small. Reasonable follow-ups:

- map and split the view-option/UI gating block;
- map camera setup separately from draw dispatch;
- defer physics and skinned avatar paths until they have dedicated maps.
