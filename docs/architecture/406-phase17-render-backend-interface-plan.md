# Phase 17 Render Backend Interface Plan

## Goal

Phase 17 starts the renderer abstraction work without changing the viewer
runtime path.

The goal is not to port OpenGL to Vulkan yet. The goal is to introduce a small
backend-neutral vocabulary for rendering intent, so later OpenGL and Vulkan
implementations can meet the same contract.

## Scope

In scope:

- add a minimal backend interface under `indra/llrender`;
- use neutral render concepts such as frame, render pass, viewport, scissor,
  clear values, load actions, and store actions;
- keep the interface unused by the runtime until a specific owner migration is
  selected.

Out of scope:

- no Vulkan dependency;
- no MoltenVK dependency;
- no runtime backend selection;
- no swapchain or OS surface ownership;
- no changes to `pipeline.cpp`;
- no changes to `LLRenderTarget`, `LLImageGL`, `LLVertexBuffer`, or shader
  execution order;
- no replacement of existing OpenGL callsites.

## First Patch

The first patch adds:

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`
- CMake wiring in `indra/llrender/CMakeLists.txt`

The interface deliberately avoids `llgltypes.h` and direct OpenGL types. It is
not a wrapper around OpenGL function names.

## Verification

Use a targeted `llrender` build first. The expected check is that the new
translation unit compiles and archives into `libllrender.a` without touching
the viewer runtime.

Also run:

- regenerated source inventory;
- OpenGL containment guardrail;
- OpenGL header-boundary guardrail;
- `git diff --check`.

## Next Small Steps

After this lands, the next safe tasks are:

- add a no-op/null backend implementation for compile-only testing;
- map one existing OpenGL owner to the new render-pass vocabulary on paper;
- only then route one tiny owner through an OpenGL backend implementation.
