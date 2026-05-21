# Phase 3 Completion Summary

This document summarizes the completed phase 3 wrapper containment work.

Branch: `phase3`

Base branch: `phase2`

## Completed Owners

Completed phase 3 owners:

- `LLRenderTarget`
- active local `LLVertexBuffer` wrapper helpers
- active `LLImageGL` wrapper, upload, allocation, and `scaleDown(...)`
  call-through helpers

## What Changed

Raw OpenGL calls were moved behind `llglcontainment.*` for:

- render target FBO binding, status, attachment, buffer routing, name lifetime,
  mipmap, clear/scissor, allocation error, and viewport calls
- vertex buffer object name lifetime, binding, storage, sub-data upload,
  attribute array/layout, and draw calls
- image pixel store, readback, framebuffer copy, scratch PBO, and texture sync
  calls
- image texture name lifetime, debug queries, residency, sub-image upload,
  texture parameters, compressed upload, auto-mipmap, manual allocation, and
  `scaleDown(...)` storage/draw calls

The changes were intentionally limited to wrapper relocation. The original
owner files still own state, branch policy, ordering, validation, accounting,
and public APIs.

## What Did Not Change

This phase did not change:

- `pipeline.cpp`
- draw pools
- UI rendering
- shader managers
- `LLImageGL` upload/mipmap/parameter/swizzle policy
- `LLImageGL::scaleDown(...)` method selection, PBO order, framebuffer copy
  order, or discard-level mutation
- texture memory accounting
- public `LLRenderTarget`, `LLVertexBuffer`, or `LLImageGL` APIs
- runtime behavior intentionally

## Verification Completed

Repeated checks across phase 3 packets:

- `git diff --check`
- targeted `llrender/fast`
- generated source inventory regeneration
- local arm64 Xcode Release build checkpoints
- arm64 executable verification
- runtime dylib presence verification

Runtime smoke:

- completed for final `LLRenderTarget` viewport packet
- intentionally deferred for later wrapper-only `LLVertexBuffer` and `LLImageGL`
  packets by project decision
- completed by user report for the `LLImageGL::scaleDown(...)` packet after
  the successful Xcode integration build

## Current Inventory Notes

Current generated inventory highlights:

- `indra/llrender/llrendertarget.cpp` has no direct `gl*` calls
- `indra/llrender/llvertexbuffer.cpp` has only disabled/commented GL work queue
  sync matches left in the generated `gl_calls` counter
- `indra/llrender/llimagegl.cpp` has no active direct `gl*` calls based on the
  local source scan; generated matches are inactive/comment-only references
- `indra/llrender/llglcontainment.cpp` now owns the raw OpenGL calls moved by
  phase 3

## Remaining Risk

Risk level: medium.

Reasons:

- runtime coverage is still smoke-level, not a broad graphics regression suite
- shader managers still contain a large direct OpenGL surface
- `llglcontainment.*` is still a containment layer, not a renderer abstraction

## Recommended Stop Point

Treat phase 3 as complete for the current wrapper-relocation objective.

Recommended next step:

- review the phase 3 branch as a stack on top of `phase2`
- decide whether the next phase should move to another owner, such as shader
  management, render orchestration, or draw-pool boundaries

Do not start broader renderer containment in `pipeline.cpp`, draw pools, UI, or
shader managers without a new phase and a stronger verification plan.
