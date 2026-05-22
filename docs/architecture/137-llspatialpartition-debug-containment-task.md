# LLSpatialPartition Debug Containment Task

Branch: `phase3`
Source owner: `LLSpatialPartition` debug rendering helpers

## Scope

Route direct OpenGL calls in `indra/newview/llspatialpartition.cpp` through
`llglcontainment.*` where they are local debug or fixed-function draw helpers.

Included raw OpenGL families:
- `glLineWidth`
- `glPolygonMode`
- `glPolygonOffset`
- `glVertexPointer`
- `glDrawElements`

Included behavior:
- octree rebuild/debug outlines
- active drawable outline width
- physics hull debug drawing
- raycast debug wireframe mode
- occlusion debug wireframe and polygon offset

## Non-Scope

Do not change:
- spatial partition traversal
- culling logic
- drawable ownership
- debug mask semantics
- shader binding order
- matrix setup
- vertex/index data ownership

Do not move any code out of `llspatialpartition.cpp`.

## Ownership Notes

`LLSpatialPartition` keeps ownership of:
- debug rendering branch selection
- traversal order
- wireframe/fill transition order
- physics hull vertex and index source data
- debug colors and line widths

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers
- fixed-function API names

## Risk

Risk: medium.

Why:
- This is wrapper-only, but the file mixes culling, debug rendering, and
  legacy fixed-function state.
- Most affected paths are debug overlays, but physics hull drawing still uses
  raw client vertex pointers.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llspatialpartition.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later with render debug overlays enabled.
