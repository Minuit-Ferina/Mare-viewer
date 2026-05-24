# Model Preview Physics Task

Date: 2026-05-24

Branch: `phase10`

## Task

Split the physics preview draw block from `LLModelPreview::render()` into a
private owner-local helper without changing render order or runtime behavior.

## Source Scope

Allowed files:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

No other source files should change in this packet.

## Helper Boundary

Add private helper:

- `renderPhysicsPreview(F32 physics_explode)`

The helper should contain the existing `show_physics` body from the
`!show_skin_weight` branch:

- depth clear;
- two pass color-mask loop;
- alpha blend state;
- physics hull/mesh selection;
- physics hull mesh build and exploded hull draw;
- physics mesh fill and edge draw;
- degenerate triangle debug draw and state restore.

## Explicit Non-Goals

Do not change:

- physics decomposition logic;
- hull versus mesh selection;
- physics explode math;
- color-mask or blend order;
- degenerate triangle detection or draw state;
- model, skinned avatar, joint, or upload behavior;
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
