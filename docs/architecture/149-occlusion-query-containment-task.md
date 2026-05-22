# Occlusion Query Containment Task

Branch: `phase3`
Source owners:
- `LLReflectionMap`
- `LLOcclusionCullingGroup`

## Scope

Route direct OpenGL occlusion query calls through `llglcontainment.*`.

Included source files:
- `indra/newview/llreflectionmap.cpp`
- `indra/newview/llvieweroctree.cpp`

Included raw OpenGL families:
- `glGenQueries`
- `glDeleteQueries`
- `glBeginQuery`
- `glEndQuery`
- `glGetQueryObjectuiv`

Included behavior:
- reflection probe occlusion queries
- octree occlusion query object pool allocation
- octree query availability and result reads
- octree query begin/end around cube bounds draw

## Non-Scope

Do not change:
- occlusion state transitions
- query pooling logic
- timeout behavior
- pending query tracking
- reflection probe occlusion policy
- draw geometry used for query bounds

Do not move occlusion ownership into another module.

## Ownership Notes

The original owners keep ownership of:
- query lifetime decisions
- query target/mode selection
- query object pooling
- availability/result interpretation
- occlusion state mutation

`llglcontainment.*` owns only:
- direct OpenGL query call-through wrappers

## Risk

Risk: medium.

Why:
- This is wrapper-only, but occlusion query timing affects visibility and
  reflection probe update decisions.
- Exact begin/end and result-read ordering must stay unchanged.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llreflectionmap.cpp.o` build
- targeted `llvieweroctree.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later in a scene with occlusion and
  reflection probes enabled.
