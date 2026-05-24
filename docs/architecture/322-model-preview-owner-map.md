# Model Preview Owner Map

Date: 2026-05-24

Branch: `phase10`

## Scope

This note maps `LLModelPreview` before source cleanup in the model upload
preview path.

No source behavior is changed by this note.

## Files Inspected

- `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`
- `docs/architecture/320-phase9-completion-summary.md`
- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llgltfmaterialpreviewmgr.h`
- `indra/newview/llgltfmaterialpreviewmgr.cpp`
- `docs/architecture/261-gltf-preview-owner-map.md`

## Owner

`LLModelPreview` owns:

- model upload preview dynamic texture participation through `ORDER_MIDDLE`;
- model upload preview camera distance, yaw, pitch, zoom, pan, and target;
- preview canvas background drawing;
- model preview LOD buffer generation;
- material diffuse texture binding for preview display;
- edge overlay drawing;
- physics preview mesh and hull drawing;
- degenerate triangle display;
- skinned avatar preview setup;
- joint override and joint position preview drawing;
- UI option mutation during preview rendering.

It does not own:

- global dynamic texture pass ordering;
- pipeline render-target allocation;
- broad `llui` layout;
- GLTF material preview texture rendering;
- BVH animation preview rendering;
- app/session lifecycle.

## Current Render Shape

`LLModelPreview::render()` currently mixes these responsibilities:

1. lock the preview object and clear `mNeedsUpdate`;
2. read view options from `mViewOption`;
3. draw the 2D preview canvas background;
4. inspect skin weights and upload flags;
5. mutate floater UI controls based on model/skinning state;
6. configure camera aspect, FOV, origin, and perspective;
7. bind `gObjectPreviewProgram`;
8. generate preview vertex buffers when needed;
9. draw ordinary model preview geometry;
10. optionally draw edge overlays;
11. optionally draw physics meshes, physics hulls, and degenerate triangles;
12. optionally configure and draw the skinned avatar preview;
13. optionally draw joint positions, bones, collision volumes, and ground plane;
14. unbind the object preview shader;
15. pop the model matrix and return `true`.

## High-Risk Coupling

High-risk coupling inside `render()`:

- UI mutation is interleaved with render state;
- buffer generation can happen during render;
- physics decomposition data is read under a different mutex;
- skinned preview changes avatar joint overrides and pelvis fixups;
- model, physics, texture, edge, avatar, and debug draws share one render
  method;
- `refresh()` is called during skinned preview to render avatar previews every
  frame;
- `gObjectPreviewProgram`, `gPhysicsPreviewProgram`, `gDebugProgram`,
  `gPipeline`, `gGL`, `LLVertexBuffer`, and `LLViewerCamera` are all borrowed.

## Lower-Risk First Boundary

The preview canvas background block is the lowest-risk source boundary.

Reasons:

- it occurs before model, physics, avatar, and UI mutation paths;
- it only draws the flat preview background;
- it already has a local matrix push/pop scope;
- it can become an owner-local helper without changing any later path.

## Source Packet Candidate

Split the 2D canvas background draw from `LLModelPreview::render()` into a
private owner-local helper.

Allowed helper:

- `drawPreviewCanvas(S32 width, S32 height)`

Allowed source files:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

Not allowed in the same packet:

- changing upload, LOD, model, physics, skinning, material, or UI behavior;
- moving UI mutation out of render;
- changing camera math;
- changing render-target or dynamic texture behavior;
- changing shader choice;
- touching `pipeline.cpp` or broad `llui`.

## Verification Plan

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

Manual smoke is deferred for this canvas-only helper split because it must
preserve call order and behavior.
