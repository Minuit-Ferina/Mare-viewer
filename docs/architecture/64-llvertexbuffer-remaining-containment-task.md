# LLVertexBuffer Remaining Containment Task

This document defines the remaining small phase 3 `LLVertexBuffer`
containment packets after buffer object name generation/deletion.

Branch: `phase3`

Base branch: `phase2`

## Task

Move the remaining raw OpenGL calls that are already isolated behind local
`LLVertexBuffer` helper functions into `llglcontainment.*`.

This is still a wrapper relocation task. `LLVertexBuffer` remains the owner of
buffer binding policy, upload ordering, attribute layout, draw validation, and
all static tracker state.

## Call Families

Move these local helper bodies only:

- `bind_vertex_buffer_target(...)`
- `allocate_vertex_buffer_storage(...)`
- `upload_vertex_buffer_sub_data(...)`
- `enable_vertex_attribute_array(...)`
- `disable_vertex_attribute_array(...)`
- `set_vertex_attribute_pointer(...)`
- `set_integer_vertex_attribute_pointer(...)`
- `draw_vertex_buffer_range(...)`
- `draw_vertex_buffer_arrays(...)`

Raw OpenGL calls to move:

- `glBindBuffer(...)`
- `glBufferData(...)`
- `glBufferSubData(...)`
- `glEnableVertexAttribArray(...)`
- `glDisableVertexAttribArray(...)`
- `glVertexAttribPointer(...)`
- `glVertexAttribIPointer(...)`
- `glDrawRangeElements(...)`
- `glDrawArrays(...)`

## Candidate API

Add narrow `LLGLContainment` helpers:

- `bindBufferObject(...)`
- `allocateBufferObjectStorage(...)`
- `updateBufferObjectSubData(...)`
- `enableVertexAttributeArray(...)`
- `disableVertexAttributeArray(...)`
- `setVertexAttributePointer(...)`
- `setIntegerVertexAttributePointer(...)`
- `drawVertexBufferRange(...)`
- `drawVertexBufferArrays(...)`

These helpers should own only the raw OpenGL calls.

## Ownership Boundary

`LLVertexBuffer` must keep ownership of:

- `sGLRenderBuffer`
- `sGLRenderIndices`
- `sLastMask`
- dirty mapped-region merging
- Apple versus non-Apple allocation behavior
- AMD generation workaround already handled by the previous packet
- buffer binding tracker update placement
- upload block splitting
- shader attribute mask decisions
- attribute offset, size, type, and normalization policy
- draw validation
- matrix sync before draw calls
- `STOP_GLERROR` placement
- `drawRangeFast(...)` fast-path differences

## Explicitly Out Of Scope

Do not touch:

- disabled GL work queue sync code
- `LLImageGL`
- `LLGLSLShader`
- shader managers
- draw pools
- UI rendering
- `indra/newview/`
- `pipeline.cpp`
- public `LLVertexBuffer` API

## Expected Behavior Change

Expected behavior change:

- none

The patch should change only where the raw OpenGL calls are issued. It should
not change call order, branch behavior, validation, or local tracker updates.

## Verification Plan

Required:

- run `git diff --check`
- build `llrender/fast`
- regenerate generated source inventory

Because the source patch touches `llglcontainment.*` and draw-related wrapper
calls, also run:

- local incremental Xcode arm64 Release build
- executable architecture check
- runtime dylib presence check

Runtime smoke:

- deferred for now by project decision on this packet family

## Stop Point

After this grouped wrapper relocation, stop and summarize the `LLVertexBuffer`
phase 3 state before choosing another owner or touching `LLImageGL`.
