# Phase 3 Containment Completion Summary

Branch: `phase3`

## Status

Phase 3 has reached the practical containment checkpoint:
- all inventoried runtime `gl*` callsites outside `llglcontainment.cpp` have
  been routed through `LLGLContainment`
- generated inventory now reports zero runtime `gl_calls` for `pipeline.cpp`
- generated inventory now reports zero runtime `gl_calls` for `llgl.cpp`
- generated inventory now reports zero runtime `gl_calls` for `llglheaders.h`

This does not mean the renderer has been refactored. It means raw OpenGL
execution is now centralized behind a narrow containment namespace.

## Inventory Snapshot

After `docs/architecture/generated/source_inventory.csv` regeneration:

- `indra/llrender/llglcontainment.cpp`: `gl_calls=160`
- `indra/newview/pipeline.cpp`: `gl_calls=0`, `gl_raw_refs=0`
- `indra/llrender/llgl.cpp`: `gl_calls=0`, raw refs remain because it still
  owns GL symbol loading and loader names
- `indra/llrender/llglheaders.h`: `gl_calls=0`, raw refs remain because it
  owns GL ABI declarations

## Work Completed

Contained major owner files:
- `LLRenderTarget`
- `LLImageGL`
- `LLVertexBuffer`
- `LLCubeMap` / `LLCubeMapArray`
- small `llrender` files
- selected viewer/UI/debug files
- FSR2 compute backend, non-Darwin only
- `llgl.cpp`
- `pipeline.cpp`

Tooling updated:
- `tools/architecture/source_inventory.py` now avoids counting comments,
  `extern gl*` prototypes, and function-pointer typedefs as runtime callsites.

## Verification Used

Repeated source packets were verified with targeted builds instead of clean
full rebuilds.

Recent final checks:
- `llrender/fast`
- targeted `pipeline.cpp.o` incremental compile
- `git diff --check`
- regenerated source inventory

No clean build was run.
No full viewer link was run after the final pipeline packet.
Runtime smoke after the final pipeline packet is still deferred.

## Remaining Short-Term Work

Recommended next small tasks:
- do one non-clean incremental viewer link checkpoint on this branch
- optionally perform a minimal manual smoke: launch, login, load one scene,
  resize, quit
- write a renderer containment contract that describes what
  `LLGLContainment` is allowed to own and what must remain with current owner
  classes
- review `todo.md` guardrails that are now obsolete because phase 3 has passed
  the original "first family only" plan

## Not Started

Still not started:
- Vulkan backend
- SDL port
- renderer file moves
- draw-pool rewrites
- UI rendering rewrite
- session lifecycle rewrite
- multi-window UI
- multi-login
