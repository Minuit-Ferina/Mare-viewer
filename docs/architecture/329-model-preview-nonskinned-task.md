# Model Preview Non-Skinned Task

Date: 2026-05-24

Branch: `phase10`

## Task

Split the ordinary non-skinned model draw loop from `LLModelPreview::render()`
into a private owner-local helper without changing render order or runtime
behavior.

## Source Scope

Allowed files:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

No other source files should change in this packet.

## Helper Boundary

Add private helper:

- `renderNonSkinnedModels(bool show_textures, bool show_edges)`

The helper should only contain the existing ordinary model draw loop for the
`!show_skin_weight` branch:

- iterate upload instances;
- select `mPreviewLOD`;
- push/pop the instance transform;
- apply preview material;
- draw model buffers;
- optionally draw edge overlay.

Physics preview drawing remains in `LLModelPreview::render()` for a separate
packet.

## Explicit Non-Goals

Do not change:

- physics preview drawing;
- skinned avatar preview drawing;
- material binding semantics;
- edge overlay state order;
- buffer generation policy;
- camera setup;
- upload or validation behavior;
- dynamic texture order or target behavior.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
