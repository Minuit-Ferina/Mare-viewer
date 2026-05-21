# Phase 3 Completion Summary

This document summarizes the completed phase 3 wrapper containment work.

Branch: `phase3`

Base branch: `phase2`

## Completed Owners

Completed phase 3 owners:

- `LLRenderTarget`
- active local `LLVertexBuffer` wrapper helpers
- local `LLImageGL` wrapper helpers already isolated in phase 2

## What Changed

Raw OpenGL calls were moved behind `llglcontainment.*` for:

- render target FBO binding, status, attachment, buffer routing, name lifetime,
  mipmap, clear/scissor, allocation error, and viewport calls
- vertex buffer object name lifetime, binding, storage, sub-data upload,
  attribute array/layout, and draw calls
- image pixel store, readback, framebuffer copy, scratch PBO, and texture sync
  calls

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

## Current Inventory Notes

Current generated inventory highlights:

- `indra/llrender/llrendertarget.cpp` has no direct `gl*` calls
- `indra/llrender/llvertexbuffer.cpp` has only disabled/commented GL work queue
  sync matches left in the generated `gl_calls` counter
- `indra/llrender/llimagegl.cpp` still has direct calls in upload, sub-image,
  mipmap, swizzle, parameter, debug texture-size, and scale-down allocation
  paths
- `indra/llrender/llglcontainment.cpp` now owns the raw OpenGL calls moved by
  phase 3

## Remaining Risk

Risk level: medium.

Reasons:

- no broad runtime graphics regression was run for the later wrapper-only
  packets
- `LLImageGL` still contains high-risk upload/mipmap/parameter paths
- shader managers still contain a large direct OpenGL surface
- `llglcontainment.*` is still a containment layer, not a renderer abstraction

## Recommended Stop Point

Treat phase 3 as complete for the current wrapper-relocation objective.

Recommended next step:

- review the phase 3 branch as a stack on top of `phase2`
- decide whether to stop here or open a separate phase for higher-risk
  `LLImageGL` upload/mipmap/parameter work

Do not start upload/mipmap/parameter containment in the same phase without a
new task note and a stronger verification plan.
