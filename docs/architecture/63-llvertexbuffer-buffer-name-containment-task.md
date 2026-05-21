# LLVertexBuffer Buffer Name Containment Task

This document defines the next phase 3 source task before editing
`llglcontainment.*` again.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llvertexbuffer.cpp`

Related contracts:

- `docs/architecture/33-llvertexbuffer-buffer-binding-update-contract.md`
- `docs/architecture/34-llvertexbuffer-phase2-review-summary.md`
- `docs/architecture/62-phase3-next-owner-selection.md`

## Task

Move only the raw `LLVertexBuffer` buffer object name lifetime OpenGL calls
behind `llglcontainment.*`, while keeping `LLVertexBuffer` as the owner of
buffer pooling, delayed deletion, platform policy, and local accounting.

This starts phase 3 work in a new owner after completing `LLRenderTarget`.
It does not create a generic vertex buffer abstraction and does not move
binding, upload, attribute, or draw behavior.

## Candidate API

Add the narrowest possible functions:

- `LLGLContainment::generateBufferObjects(...)`
- `LLGLContainment::deleteBufferObjects(...)`

Intended raw operations:

- `glGenBuffers(...)`
- `glDeleteBuffers(...)`

Reason for this shape:

- phase 2 already concentrated the raw calls in
  `generate_vertex_buffer_names(...)` and `delete_vertex_buffer_names(...)`
- the task mirrors the completed `LLRenderTarget` FBO name lifetime packet
- `LLVertexBuffer` still owns the thread-local name pool and delayed deletion
  queues
- `LLGLContainment` only issues the raw OpenGL name lifetime calls

## Ownership Boundary

`llglcontainment.*` may own:

- raw OpenGL buffer object name generation
- raw OpenGL buffer object name deletion

`LLVertexBuffer` must still own:

- `generate_vertex_buffer_names(...)`
- `delete_vertex_buffer_names(...)`
- `gen_buffer()`
- `delete_buffers(...)`
- thread-local name pool
- pool size and refill behavior
- AMD one-buffer-at-a-time workaround
- delayed deletion queues
- delayed deletion frame count policy using `LLImageGL::sFrameCount`
- `LLAppleVBOPool` versus `LLDefaultVBOPool` behavior
- buffer pool accounting
- all static binding trackers

## Source Patch Shape

Only these local helper bodies should change in the source patch:

- `generate_vertex_buffer_names(...)`
- `delete_vertex_buffer_names(...)`

Expected delegation:

- `generate_vertex_buffer_names(...)` calls
  `LLGLContainment::generateBufferObjects(...)`
- `delete_vertex_buffer_names(...)` calls
  `LLGLContainment::deleteBufferObjects(...)`
- all existing callers continue calling the local `LLVertexBuffer` helpers

No public `LLVertexBuffer` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- thread-local name pool
- `pool_size`
- `sNamePool`
- `sIndex`
- AMD one-buffer-at-a-time generation loop
- delayed deletion queues
- frame delay policy
- `gGLManager.mInited` guard
- `LLImageGL::sFrameCount` indexing
- `LLAppleVBOPool` behavior
- `LLDefaultVBOPool` behavior
- buffer binding tracker updates
- buffer allocation, upload, or flush behavior
- attribute setup
- draw calls

## Explicitly Out Of Scope

Do not move these OpenGL calls in this packet:

- `glBindBuffer(...)`
- `glBufferData(...)`
- `glBufferSubData(...)`
- `glEnableVertexAttribArray(...)`
- `glDisableVertexAttribArray(...)`
- `glVertexAttribPointer(...)`
- `glVertexAttribIPointer(...)`
- `glDrawRangeElements(...)`
- `glDrawArrays(...)`

Also do not touch:

- `indra/newview/`
- draw pools
- UI rendering
- shader managers
- texture upload code
- disabled GL work queue sync behavior

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL calls are issued, not when buffer names
are generated, pooled, queued for deletion, or actually deleted.

## Review Checklist

Reviewers should check:

- `LLGLContainment` gains only buffer object name generation/deletion helpers
- local `LLVertexBuffer` helper names still express local lifetime intent
- name pooling remains in `gen_buffer()`
- AMD workaround loop still calls the local helper once per buffer
- delayed deletion still waits through the same frame-count queue
- `gGLManager.mInited` behavior is unchanged
- no binding, upload, attribute, draw, draw-pool, UI, shader, or texture upload
  code is touched

## Verification Plan

Minimum source verification:

- run `git diff --check`
- build `llrender/fast`
- regenerate generated source inventory

Because this is a behavior-bearing `llglcontainment.*` packet, also run:

- local incremental Xcode arm64 Release build
- executable architecture check
- runtime dylib presence check

Runtime smoke after the source packet:

- login screen
- loaded scene with visible geometry
- avatar and terrain presence
- quick check for missing or scrambled geometry

## Stop Point

After the task note is committed, the next source packet should be limited to:

- adding `LLGLContainment::generateBufferObjects(...)`
- adding `LLGLContainment::deleteBufferObjects(...)`
- delegating only the two local `LLVertexBuffer` name lifetime helper bodies

Do not continue into buffer binding, upload, attribute setup, or draw calls in
the same source packet.

## Source Patch Applied

Applied in phase 3:

- added `LLGLContainment::generateBufferObjects(...)`
- added `LLGLContainment::deleteBufferObjects(...)`
- included `llglcontainment.h` in `llvertexbuffer.cpp`
- delegated `generate_vertex_buffer_names(...)` to the new containment helper
- delegated `delete_vertex_buffer_names(...)` to the new containment helper
- kept `gen_buffer()` name pooling in `LLVertexBuffer`
- kept the AMD one-buffer-at-a-time workaround in `LLVertexBuffer`
- kept delayed deletion queues and frame-count policy in `LLVertexBuffer`
- did not touch buffer binding, upload, attribute setup, draw calls, draw
  pools, UI rendering, shader managers, or texture upload code

Generated inventory was regenerated after the source patch so
`docs/architecture/generated/source_inventory.csv` and
`docs/architecture/generated/source_inventory_top.md` reflect the new call
location.

After this source patch:

- `indra/llrender/llvertexbuffer.cpp` no longer contains direct
  `glGenBuffers(...)` or `glDeleteBuffers(...)` calls
- `indra/llrender/llvertexbuffer.cpp` has 11 likely direct `gl*` calls in the
  generated inventory
- `indra/llrender/llglcontainment.cpp` has 15 likely direct `gl*` calls in the
  generated inventory

## Source Build Check

Date: 2026-05-21 CEST

Targeted build:

```sh
CLANG_MODULE_CACHE_PATH=/private/tmp/Mare-viewer-phase2-llrender-make3/clang-module-cache \
/opt/homebrew/bin/cmake \
  --build /private/tmp/Mare-viewer-phase2-llrender-make3 \
  --target llrender/fast -- -j8
```

Result: passed.

Observed work:

- rebuilt `llrender/CMakeFiles/llrender.dir/llglcontainment.cpp.o`
- rebuilt `llrender/CMakeFiles/llrender.dir/llvertexbuffer.cpp.o`
- relinked `libllrender.a`

## Integration Build Check

The phase 3 vertex buffer name containment packet was also verified with the
local incremental Xcode arm64 Release build.

Result:

- `xcodebuild -quiet` exited with code 0
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` present in the app bundle

Detailed command and output notes are recorded in:

- `docs/architecture/local-darwin-arm64-build.md`

Runtime login and loaded-scene geometry smoke still need to be run on this
exact vertex buffer name containment packet build.
