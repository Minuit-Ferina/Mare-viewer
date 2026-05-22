# Draw Pool Pass Status

Date: 2026-05-22

## Scope

This note records the current draw-pool pass-order and state-assumption map
needed to satisfy the initial "do not edit draw pools until pass order and
state assumptions are mapped" guardrail.

It is not a complete draw-pool redesign plan.

## Source Files

Draw-pool files currently present:

- `indra/newview/lldrawpool.*`
- `indra/newview/lldrawpoolalpha.*`
- `indra/newview/lldrawpoolavatar.*`
- `indra/newview/lldrawpoolbump.*`
- `indra/newview/lldrawpoolmaterials.*`
- `indra/newview/lldrawpoolpbropaque.*`
- `indra/newview/lldrawpoolsimple.*`
- `indra/newview/lldrawpoolsky.*`
- `indra/newview/lldrawpoolterrain.*`
- `indra/newview/lldrawpooltree.*`
- `indra/newview/lldrawpoolwater.*`
- `indra/newview/lldrawpoolwaterexclusion.*`
- `indra/newview/lldrawpoolwlsky.*`

## Pool Order

`LLDrawPool::POOL_*` ordering in `lldrawpool.h` is part of render behavior.
The source comment states that this order also controls render order:

- non-alpha masking/blending passes should come before other passes;
- this preserves hierarchical Z for occlusion queries;
- occlusion queries happen just before grass;
- grass should be the first alpha-masked pool;
- other ordering should consider fill rate and future occlusion likelihood.

Current pool order:

1. `POOL_SKY`
2. `POOL_WATEREXCLUSION`
3. `POOL_WL_SKY`
4. `POOL_SIMPLE`
5. `POOL_FULLBRIGHT`
6. `POOL_BUMP`
7. `POOL_MATERIALS`
8. `POOL_GLTF_PBR`
9. `POOL_TERRAIN`
10. `POOL_GRASS`
11. `POOL_GLTF_PBR_ALPHA_MASK`
12. `POOL_TREE`
13. `POOL_ALPHA_MASK`
14. `POOL_FULLBRIGHT_ALPHA_MASK`
15. `POOL_AVATAR`
16. `POOL_CONTROL_AV`
17. `POOL_GLOW`
18. `POOL_ALPHA_PRE_WATER`
19. `POOL_VOIDWATER`
20. `POOL_WATER`
21. `POOL_ALPHA_POST_WATER`
22. `POOL_ALPHA`

Guardrail: do not reorder these without a render-pipeline task and visible
scene validation.

## Render-Pass Model

`LLDrawPool` exposes separate virtual phases:

- `prerender()`
- `beginRenderPass()`
- `render()`
- `endRenderPass()`
- `beginDeferredPass()`
- `renderDeferred()`
- `endDeferredPass()`
- `beginPostDeferredPass()`
- `renderPostDeferred()`
- `endPostDeferredPass()`
- shadow pass equivalents

`LLRenderPass` defines pass IDs used by geometry registration. The source note
requires rigged variants to be adjacent to their non-rigged variants because
`LLVolumeGeometryManager::registerFace()` relies on that relation.

Guardrail: do not insert or reorder `LLRenderPass::PASS_*` values without
checking registration assumptions.

## Inventory Snapshot

Current generated inventory for the largest draw-pool files:

| file | `gl_calls` | `gGL` | `LLGL` | `LLRender` | `LLPipeline` | `LLDrawPool` | `LLViewerTexture` |
|---|---:|---:|---:|---:|---:|---:|---:|
| `lldrawpoolterrain.cpp` | 0 | 189 | 85 | 31 | 1 | 30 | 22 |
| `lldrawpoolbump.cpp` | 0 | 47 | 23 | 9 | 8 | 26 | 8 |
| `lldrawpoolavatar.cpp` | 0 | 46 | 4 | 1 | 17 | 59 | 1 |
| `lldrawpoolwlsky.cpp` | 0 | 36 | 11 | 8 | 5 | 19 | 12 |
| `lldrawpoolalpha.cpp` | 0 | 35 | 20 | 29 | 21 | 30 | 0 |
| `lldrawpool.cpp` | 0 | 21 | 13 | 11 | 0 | 49 | 3 |
| `lldrawpoolmaterials.cpp` | 0 | 11 | 8 | 3 | 0 | 9 | 0 |
| `lldrawpoolsimple.cpp` | 0 | 6 | 10 | 3 | 4 | 18 | 0 |
| `lldrawpoolwater.cpp` | 0 | 5 | 4 | 1 | 1 | 19 | 3 |
| `lldrawpooltree.cpp` | 0 | 2 | 3 | 2 | 0 | 14 | 3 |

The direct OpenGL call boundary is contained. The remaining risk is high-level
render state ownership through `gGL`, shader objects, pipeline state, texture
bindings, and pass ordering.

## Existing Focused Notes

Relevant focused docs already written:

- `docs/architecture/135-lldrawpoolterrain-fixed-function-containment-task.md`
- `docs/architecture/136-lldrawpoolterrain-fixed-function-summary.md`
- `docs/architecture/143-lldrawpoolmaterials-uniform-containment-task.md`
- `docs/architecture/144-lldrawpoolmaterials-uniform-containment-summary.md`
- `docs/architecture/81-llcubemap-containment-task.md`
- `docs/architecture/83-cubemap-containment-summary.md`
- `docs/architecture/147-debug-overlay-fixed-function-containment-task.md`
- `docs/architecture/148-debug-overlay-fixed-function-containment-summary.md`

## Risk Ranking

Highest risk for future source edits:

- `lldrawpoolterrain.cpp`: very high `gGL`, `LLGL`, `LLRender`, and texture use.
- `lldrawpoolalpha.cpp`: blend/color/deferred/post-water ordering sensitivity.
- `lldrawpoolavatar.cpp`: avatar/control-avatar pass behavior and pipeline
  coupling.
- `lldrawpoolbump.cpp`: bump/material texture state and shader/pipeline
  interactions.
- `lldrawpoolwlsky.cpp` and `lldrawpoolwater.cpp`: environment and water
  ordering/target assumptions.

Medium risk:

- `lldrawpoolmaterials.cpp`: smaller file, but material pass IDs and uniform
  state are sensitive.
- `lldrawpoolsimple.cpp`: pass fan-out and deferred/post-deferred ownership.
- `lldrawpoolpbropaque.cpp`: GLTF PBR pass behavior.
- `lldrawpooltree.cpp`: narrower but still pass-order dependent.

## Guardrail

Future draw-pool tasks must name:

- pool type;
- pass ID or phase;
- deferred/post-deferred/shadow/main path;
- shader program(s);
- texture binding assumptions;
- color/depth/blend/cull state assumptions;
- whether rigged and non-rigged variants are both affected;
- targeted object build;
- manual scene validation only when requested or when behavior changes are not
  purely mechanical containment.

Do not mix draw-pool source changes with shader manager, pipeline, or UI
rendering behavior changes in one packet.
