# Phase 3 Next Owner Selection

This document selects the next phase 3 owner after completing direct OpenGL
containment for `LLRenderTarget`.

Branch: `phase3`

Base branch: `phase2`

## Selection Goal

The next owner should be small enough to preserve the phase 3 pattern:

- start from an existing phase 2 owner contract
- move only one raw OpenGL call family
- keep local ownership and ordering in the original owner
- avoid public API changes
- avoid `pipeline.cpp`, draw pools, UI rendering, and shader manager behavior
- verify with targeted and incremental builds before moving on

## Candidate Summary

### LLImageGL

Candidate value: high.

Reason:

- owns texture lifetime, upload, readback, scratch PBOs, and sync behavior
- already has phase 2 contracts for several texture-related call families
- still has many direct OpenGL calls in `llimagegl.cpp`

Risk:

- high

Why not first:

- upload, mipmap, swizzle, and texture parameter behavior are still tightly
  coupled to format conversion, memory accounting, discard levels, and
  `LLTexUnit` binding state
- scratch PBO and sync behavior are narrow but interact with callback order and
  vendor-specific waits
- a first packet here would need stronger runtime coverage around texture-heavy
  scenes, media textures, alpha textures, compressed textures, and downscale
  paths

### LLVertexBuffer

Candidate value: high.

Reason:

- owns buffer names, binding trackers, storage, sub-data upload, attribute
  setup, and draw calls
- already has a phase 2 contract and local helper names for the relevant raw
  OpenGL calls
- the buffer name lifecycle can be split away from binding, upload, attribute
  setup, and draw behavior

Risk:

- medium for buffer name lifecycle
- medium-high to high for binding, upload, attribute layout, and draw calls

Why choose:

- the smallest first packet is only buffer name generation/deletion
- it mirrors the completed `LLRenderTarget` FBO name lifetime containment
- `LLVertexBuffer` can keep name pooling, delayed deletion timing, driver
  workaround policy, and pool accounting local
- no draw-pool, shader, or UI behavior needs to move

### LLGLSLShader / Shader Managers

Candidate value: high.

Reason:

- shader program, uniform, query, and attribute calls have a large direct
  OpenGL surface

Risk:

- high

Why not first:

- shader compile/link, bind, uniform caching, and reload policy need a focused
  owner map before phase 3 behavior moves
- uniform updates are frequent and many callsites depend on current shader
  state

### llrender.cpp

Candidate value: medium-high.

Reason:

- central texture unit, state, blending, debug, and immediate-style render
  logic lives here

Risk:

- high

Why not first:

- it is a central low-level wrapper already used by many owners
- selecting it now would blur phase 3 from owner-specific containment into a
  broader OpenGL wrapper pass

### pipeline.cpp

Candidate value: very high.

Risk:

- very high

Decision:

- do not choose for the next phase 3 packet

Reason:

- frame orchestration remains too central for early containment
- lower-level owners should be reduced and documented before touching pipeline
  behavior

## Decision

Choose `LLVertexBuffer` as the next phase 3 owner.

Choose only the buffer name lifecycle as the first source packet.

The next source packet should move only these raw OpenGL calls:

- `glGenBuffers(...)`
- `glDeleteBuffers(...)`

The expected affected local helpers are:

- `generate_vertex_buffer_names(...)`
- `delete_vertex_buffer_names(...)`

## Ownership To Preserve

`LLVertexBuffer` must keep ownership of:

- thread-local buffer name pool
- pool size and refill behavior
- AMD one-buffer-at-a-time workaround
- delayed deletion queues
- delayed deletion frame count policy using `LLImageGL::sFrameCount`
- `LLAppleVBOPool` versus `LLDefaultVBOPool` behavior
- buffer pool accounting
- `sGLRenderBuffer` and `sGLRenderIndices` tracker update placement

The containment helper should own only the raw OpenGL call.

## Explicitly Out Of Scope

Do not move these in the first `LLVertexBuffer` phase 3 packet:

- `glBindBuffer(...)`
- `glBufferData(...)`
- `glBufferSubData(...)`
- `glEnableVertexAttribArray(...)`
- `glDisableVertexAttribArray(...)`
- `glVertexAttribPointer(...)`
- `glVertexAttribIPointer(...)`
- `glDrawRangeElements(...)`
- `glDrawArrays(...)`
- disabled or optional GL work queue sync behavior
- static binding tracker updates
- dirty-region merge or flush behavior
- attribute mask setup
- draw validation or matrix sync

## Verification Plan

Before source edits:

- add a task-specific containment note for the `LLVertexBuffer` buffer name
  lifecycle

For the source packet:

- run `git diff --check`
- run targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`
- regenerate `docs/architecture/generated/source_inventory_top.md`
- run the local incremental Xcode arm64 Release build
- verify the executable is arm64
- verify runtime dylibs are present in the app bundle

Runtime smoke after the source packet:

- login screen
- loaded scene with visible geometry
- avatar and terrain presence
- quick check for missing or scrambled geometry

## Stop Point

The next immediate step is documentation only:

- add `docs/architecture/63-llvertexbuffer-buffer-name-containment-task.md`

Do not edit source until that task note exists.
