# Pipeline Containment Task

Branch: `phase3`
Source owner: frame orchestration and render pass coordination

## Scope

Address the final direct runtime OpenGL callsites in:
- `indra/newview/pipeline.cpp`

Included call families:
- query lifetime and transform-feedback query markers
- texture parameter updates
- clear color and clear masks
- polygon mode, polygon offset, line width, and point size
- framebuffer status check
- viewport restore and post-processing viewport changes
- avatar velocity matrix upload
- draw buffer selection

## Non-Scope

Do not change:
- render pass order
- render target ownership
- shader selection
- texture binding order
- viewport dimensions
- debug rendering behavior
- pathfinding rendering behavior
- avatar impostor rendering behavior

Do not move logic out of `pipeline.cpp` in this packet.

## Ownership Notes

`pipeline.cpp` still owns frame orchestration and render pass sequencing.

`llglcontainment.*` owns only the direct OpenGL call-through wrappers.

The intended source change is containment-only: replace executable `gl*`
calls with existing or narrow `LLGLContainment` helpers.

## Risk

Risk: medium-high.

Why:
- `pipeline.cpp` is the central frame orchestration file.
- The callsites span main render, debug overlays, pathfinding overlays,
  deferred lighting, post-processing, and impostor rendering.
- The patch is wrapper-only, but the blast radius is broad.

## Verification

Required:
- `git diff --check`
- targeted/incremental `newview` compile for `pipeline.cpp`
- regenerate `docs/architecture/generated/source_inventory.csv`

Optional:
- login smoke
- scene load and resize smoke

No clean build unless explicitly requested.
