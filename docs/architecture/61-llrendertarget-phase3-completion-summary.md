# LLRenderTarget Phase 3 Completion Summary

This document closes the phase 3 direct OpenGL containment pass for
`LLRenderTarget`.

Branch: `phase3`

Base branch: `phase2`

## Purpose

The purpose of this phase 3 packet was to move selected raw OpenGL calls used
by `LLRenderTarget` behind `llglcontainment.*` while keeping render target
ownership, ordering, policy, and public APIs unchanged.

This was not a renderer abstraction rewrite. It was a narrow containment pass
for already-mapped phase 2 helper boundaries.

## Completed Packets

Completed behavior-bearing containment packets:

- FBO bind/status:
  - `LLGLContainment::bindReadWriteFramebuffer(...)`
  - `LLGLContainment::getDrawFramebufferStatus(...)`
- FBO texture attachment:
  - `LLGLContainment::setReadWriteFramebufferTexture2D(...)`
- draw/read buffer routing:
  - `LLGLContainment::setDrawBuffer(...)`
  - `LLGLContainment::setReadBuffer(...)`
  - `LLGLContainment::setDrawBuffers(...)`
- FBO name lifetime:
  - `LLGLContainment::generateFramebuffers(...)`
  - `LLGLContainment::deleteFramebuffers(...)`
- mipmap generation:
  - `LLGLContainment::generateTextureMipmap(...)`
- clear/scissor:
  - `LLGLContainment::clearBuffers(...)`
  - `LLGLContainment::setScissorBox(...)`
- allocation error read:
  - `LLGLContainment::getError()`
- viewport:
  - `LLGLContainment::setViewport(...)`

## Source Scope

Touched source files across the completed packet:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Touched generated files across the completed packet:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

No other render owners were changed as part of this pass.

## Final Inventory Result

Current result:

- `indra/llrender/llrendertarget.cpp` has no direct `gl*` calls.
- the selected raw OpenGL calls now live in `indra/llrender/llglcontainment.*`.
- `indra/llrender/llglcontainment.cpp` owns the 13 raw OpenGL calls moved by
  this phase 3 render target pass.

## Ownership Preserved

`LLRenderTarget` still owns:

- `sCurFBO`, `sBoundTarget`, `sCurResX`, and `sCurResY`
- `bindTarget()` / `flush()` stack ordering
- default framebuffer restore behavior
- framebuffer status warning and failure policy
- color/depth attachment decisions
- attachment index math
- texture name `0` detach policy
- depth-only `GL_NONE` routing
- default framebuffer `GL_BACK` routing
- `mFBO` ownership and release ordering
- mipmap policy and attachment binding
- clear mask selection
- scissor enable scope
- `stop_glerror()` placement
- allocation error boolean policy
- allocation warning and return behavior
- `gGLViewport` reads
- viewport tracking updates

No public `LLRenderTarget` API changed.

## Areas Not Touched

This pass did not edit:

- `indra/newview/pipeline.cpp`
- draw pools
- UI rendering
- shader managers
- texture upload code
- viewer window code
- camera code
- probe code
- public render target call sites

## Verification Completed

Each source packet was checked with:

- `git diff --check`
- targeted `llrender/fast`
- generated source inventory regeneration
- local incremental Xcode arm64 Release build
- arm64 executable verification
- runtime dylib presence verification

Runtime smoke coverage recorded during the pass:

- login screen after the attachment packet
- loaded scene after the buffer routing packet
- login screen after the clear/scissor packet
- loaded-scene and resize smoke before viewport containment
- login, loaded-scene, and resize smoke after viewport containment

## Expected Behavior Change

Expected behavior change:

- none

The pass changed where raw OpenGL calls are issued. It did not change which
calls are made, call order, render target ownership, or render target policy.

## Remaining Risk

Risk level: low to medium.

Remaining risk:

- no formal FPS recapture after the completed phase 3 render target pass
- no broad graphics regression pass
- no focused dynamic texture, probe, or preview-widget pass
- `llglcontainment.*` is still a small OpenGL containment point, not a renderer
  abstraction

## Next Owner Candidates

Reasonable next-owner candidates:

- `LLImageGL`: high value because it owns texture lifetime and upload, but
  higher risk because upload, readback, parameters, and deletion interact with
  many viewer paths.
- `LLVertexBuffer`: high value because it owns buffers, attributes, and draw
  calls, but performance-sensitive and easy to disturb accidentally.
- `LLGLSLShader` / shader managers: high value, but shader compile, bind, and
  uniform policy need a very narrow first packet.
- `llrender.cpp`: central immediate-mode/state wrapper area; useful, but it
  should be split carefully by intent.

`pipeline.cpp` should not be the next phase 3 owner. It remains too central for
early direct-call containment and should stay protected until lower-level owner
contracts are clearer.

## Stop Point

Treat the `LLRenderTarget` phase 3 containment pass as complete.

Before the next source patch, add a short next-owner selection note that
compares the candidate owner, exact call family, ownership policy, risk, and
verification plan.
