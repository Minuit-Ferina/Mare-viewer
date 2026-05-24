# Model Preview Non-Skinned Summary

Date: 2026-05-24

Branch: `phase10`

## Source Changes

Files changed:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

`LLModelPreview::render()` now delegates the ordinary non-skinned model draw
loop to:

- `renderNonSkinnedModels(bool show_textures, bool show_edges)`

The helper keeps the existing order:

1. the caller remains inside the `!show_skin_weight` branch;
2. upload instances are iterated in the same order;
3. `mPreviewLOD` model selection still skips null models;
4. instance transform push, multiply, and pop remain around the same draws;
5. preview material application remains immediately before the fill draw;
6. vertex-buffer setup and fill draw order is unchanged;
7. texture unbind, edge color, edge polygon mode, line width, and restore order
   are unchanged.

## Behavior Notes

No runtime behavior is intentionally changed.

This packet does not touch:

- physics preview drawing;
- skinned avatar preview drawing;
- material binding semantics;
- edge overlay state order;
- buffer generation policy;
- camera setup;
- upload or validation behavior;
- dynamic texture order or target behavior.

## Verification

Passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

## Residual Risk

The non-skinned branch still contains the physics preview block in
`LLModelPreview::render()`.

The next useful boundary is to split physics preview drawing as one coherent
helper, then split skinned avatar preview as a separate coherent helper.
