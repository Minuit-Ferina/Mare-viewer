# Model Preview Canvas Task

Date: 2026-05-24

Branch: `phase10`

## Task

Split the `LLModelPreview::render()` 2D canvas background draw into a private
owner-local helper without changing render order or runtime behavior.

## Source Scope

Allowed files:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

No other source files should change in this packet.

## Helper Boundary

Add private helper:

- `drawPreviewCanvas(S32 width, S32 height)`

The helper should only contain the existing preview canvas setup and draw:

- bind `gUIProgram`;
- push and load projection/modelview matrices;
- set orthographic projection;
- draw `PREVIEW_CANVAS_COL`;
- pop projection/modelview matrices;
- unbind `gUIProgram`.

## Required Ordering

Preserve this order:

1. lock the preview object;
2. clear `mNeedsUpdate`;
3. read current view options;
4. read preview width and height;
5. create existing UI/default, no-blend, cull, and disabled-depth scopes;
6. draw the preview canvas;
7. continue into all existing model, UI, physics, avatar, and shader work.

## Explicit Non-Goals

Do not change:

- upload or validation behavior;
- LOD generation;
- model, material, physics, skinned avatar, joint, or debug drawing;
- UI mutation inside render;
- camera math;
- shader choices;
- dynamic texture order or target behavior;
- `pipeline.cpp` or broad `llui`.

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
