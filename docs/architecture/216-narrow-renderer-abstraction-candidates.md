# Narrow Renderer Abstraction Candidates

Date: 2026-05-22

## Scope

This note considers narrow abstractions after the renderer ownership contract.

It does not propose a source patch. It separates candidates that may be worth a
future task from candidates that are still too broad.

## Acceptable Candidate Shape

A candidate is acceptable only if it:

- has one owner file or owner subsystem;
- removes repeated intent, not just repeated spelling;
- preserves ordering and restore behavior;
- can be verified with a targeted build;
- has a named runtime surface when behavior could be visible.

## Candidate 1: Dynamic Texture Render Scope

Potential owner:

- `LLViewerDynamicTexture`

Problem:

- `updateAllInstances()` owns target selection, viewport setup, camera
  save/restore, per-instance render calls, and framebuffer-to-texture copy.

Possible narrow abstraction:

- a local helper/scope inside `lldynamictexture.cpp` that documents and enforces
  target selection and restore ordering.

Why not now:

- `LLViewerTexLayerSetBuffer`, `LLVisualParamReset`, and
  `LLPreviewAnimation` have special behavior that needs runtime observation or
  a very focused task before source changes.

Status:

- good candidate for a future task note, not an immediate source patch.

## Candidate 2: UI Clip Rect Contract

Potential owner:

- `LLLocalClipRect` / `LLScreenClipRect`

Problem:

- UI clipping owns scissor behavior and must flush `gGL` before changing
  scissor state.

Possible narrow abstraction:

- a named scissor update helper that keeps flush and restore behavior explicit.

Why not now:

- header exposure has already been cleaned up;
- runtime behavior is stable;
- any next change should have a focused UI clipping smoke surface.

Status:

- possible, but low urgency.

## Candidate 3: Map Tracking Drawing Helpers

Potential owner:

- `LLWorldMapView` static tracking helpers

Problem:

- minimap and world map share tracking circle/dot/arrow helpers.

Possible narrow abstraction:

- a map tracking overlay helper local to map UI code.

Why not now:

- this is UI behavior, not OpenGL containment;
- validation requires visible minimap/world-map tracking states.

Status:

- possible later, after map behavior is explicitly in scope.

## Candidate 4: Draw-Pool Pass State Object

Potential owner:

- individual draw-pool task, not all draw pools

Problem:

- draw pools combine pass IDs, shader selection, texture bindings, blend/depth
  state, and render order.

Possible narrow abstraction:

- a helper for one exact pass family, such as a material uniform setup or one
  alpha pass state block.

Why not now:

- draw-pool behavior has high visible risk;
- pass order and shader state should not be generalized across pools.

Status:

- only acceptable as a one-pool, one-pass task.

## Rejected For Now

Do not start these as abstractions:

- generic renderer backend interface;
- generic texture abstraction over `LLImageGL`, `LLViewerTexture`,
  `LLRenderTarget`, and GLTF previews;
- generic shader abstraction over `LLGLSLShader`, `LLShaderMgr`, and
  `LLViewerShaderMgr`;
- generic draw-pool renderer;
- SDL windowing port;
- Vulkan backend;
- multi-window architecture;
- login/session lifecycle rewrite.

## Recommended Next Source Task Shape

If source work resumes, use this shape:

1. Pick one owner file.
2. Write a task note with the exact call family or behavior.
3. Keep the patch below one behavioral concept.
4. Run the relevant guardrails.
5. Run a targeted build.
6. Ask for manual runtime validation only if visible behavior changed.

Current best candidates are documentation-first:

- dynamic texture render-scope task note;
- UI clip rect scissor task note;
- one draw-pool pass task note.
