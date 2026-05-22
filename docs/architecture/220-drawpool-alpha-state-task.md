# Draw Pool Alpha State Task

Date: 2026-05-22

## Scope

This is a future source task note for `LLDrawPoolAlpha`.

It does not propose a source patch. It constrains any future alpha draw-pool
cleanup or state helper extraction.

## Owner Files

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Current Role

`LLDrawPoolAlpha` owns post-deferred forward alpha rendering. It handles:

- pre-water and post-water alpha pool behavior;
- water-plane clipping uniforms;
- HUD-specific alpha behavior;
- impostor alpha behavior;
- rigged and non-rigged alpha rendering;
- GLTF scene depth pre-rendering before rigged post-water alpha;
- depth-only alpha pass for depth-of-field;
- debug alpha highlighting;
- alpha blend factors and color mask state;
- per-draw texture/material setup.

## Existing High-Level Flow

`renderPostDeferred()`:

1. Returns early for pre-water alpha when water clipping says alpha on the other
   side of the water plane should not render.
2. Computes `water_sign` from pool type and underwater state.
3. Prepares emissive, PBR emissive, fullbright, simple, material, and PBR alpha
   shaders.
4. Unbinds the current shader so the render loop does not depend on the last
   prepared shader.
5. For non-HUD rendering, calls `forwardRender(true)` for a rigged/depth pass.
6. Calls `forwardRender()` for normal forward alpha rendering.
7. Optionally performs the depth-of-field depth-only pass for
   `POOL_ALPHA_POST_WATER`.

`forwardRender(bool rigged)`:

1. Enables dynamic lights.
2. Creates `LLGLSPipelineAlpha`.
3. Enables alpha writes through `gGL.setColorMask(true, true)`.
4. Computes whether depth writes are enabled.
5. Creates `LLGLDepthTest`.
6. Sets the normal alpha blend factors.
7. For rigged post-water, renders GLTF scene depth before rigged alpha.
8. Calls `renderAlpha(...)`.
9. Restores color mask to color-on/alpha-off.
10. For the final non-rigged pass, renders debug alpha while pipeline alpha and
    depth state are still in scope.

## State That Must Stay Explicit

Do not hide these behind a generic draw-pool abstraction:

- `mColorSFactor`
- `mColorDFactor`
- `mAlphaSFactor`
- `mAlphaDFactor`
- `mRigged`
- `target_shader`
- `simple_shader`
- `fullbright_shader`
- `emissive_shader`
- `pbr_emissive_shader`
- `pbr_shader`
- `sWaterPlane`

These are not just GL state. They carry alpha pass intent.

## Fragile Ordering

Do not reorder these without a dedicated behavior task:

- shader preparation before render loop;
- explicit `LLGLSLShader::unbind()` after shader preparation;
- rigged depth pass before regular alpha;
- `renderDebugAlpha()` inside `forwardRender(false)`;
- GLTF scene depth render before rigged post-water alpha;
- depth-of-field depth-only pass after normal forward alpha;
- final `gGL.setSceneBlendType(LLRender::BT_ALPHA)` in `renderAlpha()`;
- per-draw blend override and restore inside `renderAlpha()`.

## Candidate Future Helper

The only reasonable first helper would be local to `lldrawpoolalpha.cpp`, such
as a named helper for alpha pass setup that keeps:

- water sign;
- shader preparation;
- blend factors;
- depth write decision;
- color mask restore;

in the same owner file.

Do not introduce a generic draw-pool pass object from this file.

## Verification Plan

For a source patch:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Manual runtime validation is required for any behavior change:

- scene with transparent objects above water;
- scene with transparent objects around water;
- rigged alpha attachments;
- HUD alpha attachments;
- impostor rendering if practical;
- depth-of-field alpha interaction if enabled;
- debug alpha mode if touched.

## Stop Conditions

Stop before source changes if the intended patch would:

- alter alpha pool order;
- alter pre-water/post-water semantics;
- change shader selection;
- change blend factors;
- change depth write decisions;
- combine alpha cleanup with shader manager or pipeline changes;
- generalize behavior across draw pools.
