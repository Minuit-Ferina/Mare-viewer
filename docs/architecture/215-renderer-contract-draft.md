# Renderer Contract Draft

Date: 2026-05-22

## Scope

This draft records the current renderer ownership boundaries after the
inventory, containment, and header-boundary work.

It is not a backend abstraction design. It is the minimum contract needed before
considering narrow abstractions.

## Conceptual Ownership

| area | current owner examples | contract |
|---|---|---|
| frame orchestration | `LLViewerDisplay`, `LLViewerWindow`, `LLPipeline` | decides when startup, UI, scene, deferred, dynamic texture, and presentation work happen |
| low-level GL boundary | `LLGLContainment`, `LLGL`, `LLGLHeaders`, `LLGLState`, `LLRender` | owns raw OpenGL call-through, loader declarations, and immediate render state |
| render targets | `LLRenderTarget`, `LLPipeline` targets, dynamic texture targets | owns FBO/texture attachments, bind/flush ordering, viewport/clear behavior |
| textures | `LLImageGL`, `LLViewerTexture`, fetched/local textures, GLTF preview textures | owns GL texture names, upload, cache/readback, preview texture copy paths |
| shaders | `LLGLSLShader`, `LLShaderMgr`, `LLViewerShaderMgr`, shader assets | owns compile/link/load, feature flags, generated defines, program variants, uniforms |
| render passes | `LLDrawPool`, `LLRenderPass`, pool subclasses, `pipeline.cpp` | owns pool/pass ordering, shader selection, per-pass texture/state assumptions |
| UI rendering | `llui`, floaters, maps, previews, dynamic textures | may request rendering, draw immediate geometry, or borrow render targets; should not own backend GL policy |
| platform windowing | `llwindow`, app/viewer launch code, native app bundle resources | owns native windows, contexts, input, app lifecycle, and platform packaging quirks |

## Current Hard Boundaries

- New runtime `gl*` calls must not appear outside
  `indra/llrender/llglcontainment.cpp`.
- Runtime headers must not include `llgl.h` directly outside the
  precompiled/prefix headers.
- Raw GL scalar type spellings in runtime headers are confined to
  `llglheaders.h`.
- FSR2 remains disabled on Darwin.
- Native Linden/Kokua windowing remains the platform strategy for now.
- No Vulkan backend and no SDL port are part of the current plan.

## UI Seams

UI code requests or performs rendering through these seams:

- core `llui` widgets drawing through `gGL`, `LLUIImage`, and font helpers;
- `LLLocalClipRect` owning UI scissor behavior;
- `LLViewerDynamicTexture` borrowing pipeline render targets for preview and
  bake content;
- minimap/world map controls drawing map textures, overlays, and tracking
  helpers directly;
- upload/model/material/snapshot/animation previews mixing UI workflow state
  with scene-like rendering.

The UI should be treated as several render clients, not one UI renderer.

## World/UI Coupling Seams

World rendering still depends on viewer-global or UI-adjacent state in these
areas:

- `pipeline.cpp` owns high-level pass orchestration and many global render
  decisions.
- draw pools depend on global shader handles, pipeline state, texture state,
  and pass ordering.
- map widgets render world/map information inside UI controls.
- dynamic texture previews borrow render targets from `gPipeline`.
- snapshot preview and capture paths bridge UI settings with render output.

These seams should be documented before any behavior is moved.

## Texture Lifetime Seams

Texture lifetime crosses:

- `LLImageGL` GL names and upload/readback behavior;
- `LLViewerTexture` dynamic/fetched/local texture ownership;
- `LLRenderTarget` attachments;
- dynamic texture framebuffer-to-texture copies;
- GLTF material preview texture loading;
- map overlay local textures;
- upload and preview floaters.

Do not introduce a generic texture abstraction until a task names which of
these lifetimes it owns.

## Shader Lifetime Seams

Shader lifetime crosses:

- source asset selection and class fallback;
- generated defines and feature flags;
- compile/link/validate behavior;
- binary cache load/save;
- GLTF variant creation;
- viewer global shader handles;
- draw-pool and pipeline pass ownership;
- FSR2/TAA/NIS/Mare upscaler paths.

Shader manager changes should be isolated from draw-pool or pipeline behavior
changes unless a task explicitly requires both.

## Render Pass Dependencies

Render pass order depends on:

- `LLDrawPool::POOL_*` order;
- `LLRenderPass::PASS_*` numeric relationships, especially rigged variants;
- deferred vs post-deferred vs shadow pass phase;
- alpha, water, glow, terrain, avatar, GLTF, and material ordering;
- `pipeline.cpp` orchestration;
- shader and texture state assumptions inside each pool.

Do not reorder passes as cleanup.

## Narrow Abstraction Rule

A new abstraction is acceptable only when it:

- replaces a documented ownership boundary;
- has one clear owner;
- removes a real repeated pattern;
- does not hide UI/render or platform-specific behavior;
- can be validated with a targeted build and a named runtime surface when
  behavior changes.

The next renderer abstraction should be proposed as a task note first, not as a
source patch.
