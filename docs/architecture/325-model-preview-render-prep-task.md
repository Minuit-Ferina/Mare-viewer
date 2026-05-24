# Model Preview Render Prep Task

Date: 2026-05-24

Branch: `phase10`

## Task

Split several preparation blocks in `LLModelPreview::render()` into private
owner-local helpers without changing render order or runtime behavior.

## Source Scope

Allowed files:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

No other source files should change in this packet.

## Helper Boundary

Add private helpers:

- `updateSkinPreviewControls(...)`
- `ensurePreviewLODVertexBuffers(...)`
- `applyPreviewMaterial(...)`

The helpers should only name existing blocks:

- upload skin/joint UI gating and skin-weight detection;
- preview LOD/physics vertex-buffer regeneration checks;
- duplicated preview material diffuse color and texture binding.

## Required Ordering

Preserve this order:

1. draw the preview canvas;
2. read upload skin and upload joints control values;
3. update skin preview controls and return `has_skin_weights`;
4. read `physics_explode`;
5. configure depth, camera, shader, and lights;
6. generate base model buffers if needed;
7. when the preview LOD has models, enable reset, ensure LOD/physics buffers,
   then continue existing non-skinned or skinned draw paths;
8. keep material application immediately before the same buffer draw calls as
   before.

## Explicit Non-Goals

Do not change:

- upload or validation behavior;
- skin/joint control semantics;
- LOD generation policy;
- physics generation or rendering;
- skinned avatar rendering;
- camera math;
- shader choices;
- edge overlay drawing;
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
