# LLVertexBuffer Phase 3 Completion Summary

This document closes the phase 3 direct OpenGL containment pass for the active
`LLVertexBuffer` local wrapper helpers.

Branch: `phase3`

Base branch: `phase2`

## Purpose

The purpose of this pass was to move raw OpenGL calls already isolated behind
local `LLVertexBuffer` helper functions into `llglcontainment.*`, while keeping
`LLVertexBuffer` as the owner of buffer state, upload ordering, attribute
layout, draw validation, and tracker policy.

This was a wrapper relocation pass, not a vertex buffer rewrite.

## Completed Packets

Completed behavior-bearing containment packets:

- buffer object name lifetime:
  - `LLGLContainment::generateBufferObjects(...)`
  - `LLGLContainment::deleteBufferObjects(...)`
- buffer binding:
  - `LLGLContainment::bindBufferObject(...)`
- buffer storage and upload:
  - `LLGLContainment::allocateBufferObjectStorage(...)`
  - `LLGLContainment::updateBufferObjectSubData(...)`
- vertex attribute arrays:
  - `LLGLContainment::enableVertexAttributeArray(...)`
  - `LLGLContainment::disableVertexAttributeArray(...)`
- vertex attribute layout:
  - `LLGLContainment::setVertexAttributePointer(...)`
  - `LLGLContainment::setIntegerVertexAttributePointer(...)`
- draw calls:
  - `LLGLContainment::drawVertexBufferRange(...)`
  - `LLGLContainment::drawVertexBufferArrays(...)`

## Source Scope

Touched source files across the completed pass:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llvertexbuffer.cpp`

Touched generated files:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

No public `LLVertexBuffer` API changed.

## Ownership Preserved

`LLVertexBuffer` still owns:

- buffer name pooling
- AMD one-buffer-at-a-time generation workaround
- delayed deletion queues
- delayed deletion frame-count policy
- `LLAppleVBOPool` versus `LLDefaultVBOPool` behavior
- `sGLRenderBuffer`
- `sGLRenderIndices`
- `sLastMask`
- dirty mapped-region merging
- upload block splitting
- Apple versus non-Apple upload behavior
- shader attribute mask decisions
- attribute offset, size, type, and normalization policy
- draw validation
- matrix sync before draw calls
- `STOP_GLERROR` placement
- `drawRangeFast(...)` fast-path differences

`llglcontainment.*` owns only the raw OpenGL call sites moved by this phase 3
pass.

## Areas Not Touched

This pass did not edit:

- disabled GL work queue sync code
- `LLImageGL`
- `LLGLSLShader`
- shader managers
- draw pools
- UI rendering
- `indra/newview/`
- `pipeline.cpp`

## Final Inventory Result

After this pass:

- `indra/llrender/llvertexbuffer.cpp` has 2 likely direct `gl*` calls in the
  generated inventory
- those 2 remaining matches are from disabled/commented GL work queue sync
  context
- the active local `LLVertexBuffer` wrapper helper bodies no longer issue raw
  OpenGL calls directly
- `indra/llrender/llglcontainment.cpp` has 24 likely direct `gl*` calls in the
  generated inventory

## Verification Completed

Completed checks:

- `git diff --check`
- targeted `llrender/fast`
- generated source inventory regeneration
- local incremental Xcode arm64 Release build
- arm64 executable verification
- runtime dylib presence verification

Runtime smoke was intentionally deferred for this wrapper-only packet family.

## Expected Behavior Change

Expected behavior change:

- none

The pass changed where raw OpenGL calls are issued. It did not change local
call order, branch behavior, tracker updates, validation, upload policy, or
draw policy.

## Remaining Risk

Risk level: medium.

Reasons:

- buffer binding and draw calls are performance-sensitive
- attribute layout still depends on shader reserved attribute ordering
- static trackers still need to match real OpenGL state
- runtime smoke was deferred by project decision

The risk is bounded because the source patch only delegated already-isolated
local helper bodies to `llglcontainment.*`.

## Stop Point

Treat the active `LLVertexBuffer` phase 3 wrapper relocation pass as complete.

Before touching another owner, add a short next-owner selection note. Good next
candidates are `LLImageGL` local wrapper helpers or a focused shader ownership
map, but neither should start directly from this summary commit.
