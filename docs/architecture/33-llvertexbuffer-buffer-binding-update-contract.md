# LLVertexBuffer Buffer Binding And Update Contract

This document starts the phase 2 review of `LLVertexBuffer` buffer ownership,
binding, update, attribute setup, and draw behavior. It is documentation only
and does not change source behavior.

Related files:

- `indra/llrender/llvertexbuffer.cpp`
- `indra/llrender/llvertexbuffer.h`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/04-gl-callsite-inventory.md`

## Inventory

Generated inventory row:

- file: `indra/llrender/llvertexbuffer.cpp`
- category: `render.legacy_low_level`
- lines: 1912
- likely `gl*` calls: 40
- raw `gl*` references: 46
- `gGL` references: 33
- `LLGL` references: 10

Local text scan of OpenGL names in `llvertexbuffer.cpp`:

| call family | count |
|---|---:|
| `glVertexAttribPointer` | 13 |
| `glBindBuffer` | 11 |
| `glBufferSubData` | 6 |
| `glGenBuffers` | 3 |
| `glBufferData` | 3 |
| `glVertexAttribIPointer` | 2 |
| `glDrawRangeElements` | 2 |
| `glEnableVertexAttribArray` | 1 |
| `glDisableVertexAttribArray` | 1 |
| `glDrawArrays` | 1 |
| `glDeleteBuffers` | 1 |

Notes:

- `glFenceSync` and `glWaitSync` appear only in disabled `ENABLE_GL_WORK_QUEUE`
  code/comment context.
- `glMapBuffer`/`glUnmapBuffer` are declared in GL headers but are not used by
  current `LLVertexBuffer` code.
- `LLVertexBuffer` uses CPU-side mapped memory plus explicit flush/copy paths,
  not direct OpenGL map/unmap calls.

## Primary Responsibilities

`LLVertexBuffer` owns several overlapping responsibilities:

- GL buffer name pooling and delayed deletion
- VBO/IBO allocation through platform-specific pools
- CPU-side mapped vertex/index memory
- dirty region tracking for mapped vertex/index ranges
- streaming updates through `glBufferSubData(...)`
- Apple-specific buffer reallocation on unmap
- current GL array/index buffer tracking through static state
- shader attribute array enable/disable state
- shader attribute pointer layout
- indexed and non-indexed draw calls

Treat it as several smaller contracts, not one source patch.

## Buffer Name Lifecycle

Likely owner functions/classes:

- `gen_buffer()`
- `delete_buffers(...)`
- `LLAppleVBOPool`
- `LLDefaultVBOPool`
- `LLVertexBuffer::destroyGLBuffer()`
- `LLVertexBuffer::destroyGLIndices()`

OpenGL families:

- `glGenBuffers`
- `glDeleteBuffers`

Current behavior:

- `gen_buffer()` batches buffer-name generation through a thread-local pool
- non-Darwin non-AMD paths can call `glGenBuffers(pool_size, ...)`
- AMD workaround path generates one buffer name at a time
- `delete_buffers(...)` delays deletion by a few frames using
  `LLImageGL::sFrameCount`
- default VBO pool reuses GL names and CPU-side aligned memory
- Apple VBO pool effectively disables VBO pooling and delays GL allocation
  until unmap

Risk level: high.

Reasons:

- GL buffer names are shared with static binding trackers
- delayed deletion relies on frame count and GPU completion assumptions
- default and Apple pool paths intentionally differ
- pool accounting and GL object lifetime must stay aligned

## Buffer Allocation And Upload

Likely owner functions/classes:

- `LLDefaultVBOPool::allocate(...)`
- `LLAppleVBOPool::allocate(...)`
- `LLVertexBuffer::genBuffer(...)`
- `LLVertexBuffer::genIndices(...)`
- `LLVertexBuffer::createGLBuffer(...)`
- `LLVertexBuffer::createGLIndices(...)`
- `LLVertexBuffer::_unmapBuffer()`
- `LLVertexBuffer::flush_vbo(...)`

OpenGL families:

- `glBindBuffer`
- `glBufferData`
- `glBufferSubData`

Current behavior:

- default pool cache miss generates a buffer, binds it, and allocates GPU
  storage with `glBufferData(..., nullptr, GL_DYNAMIC_DRAW)`
- default pool also updates either `sGLRenderBuffer` or `sGLRenderIndices`
  after binding
- Apple path allocates aligned CPU memory first and creates/recreates GL buffer
  storage during `_unmapBuffer()`
- `flush_vbo(...)` copies to CPU mapped memory on Apple and streams ranges with
  `glBufferSubData(...)` elsewhere
- non-Apple `flush_vbo(...)` assumes the correct array or index buffer is
  already bound through `sGLRenderBuffer`/`sGLRenderIndices`
- dirty mapped regions are merged and flushed in `_unmapBuffer()`
- full-setter methods and offset-setter methods both route through
  `flush_vbo(...)`

Risk level: high.

Reasons:

- array and element-array buffer bindings are global OpenGL state
- dirty-region merging controls exactly which bytes are uploaded
- Apple and non-Apple update paths have different allocation/copy semantics
- `glBufferSubData(...)` splits updates into fixed-size chunks
- index buffers can switch from 16-bit to 32-bit through `setIndexData(...)`

## Binding State Ownership

Likely owner functions:

- `LLVertexBuffer::unbind()`
- `LLVertexBuffer::setBuffer()`
- `LLVertexBuffer::_unmapBuffer()`
- `LLDefaultVBOPool::allocate(...)`

State:

- `LLVertexBuffer::sGLRenderBuffer`
- `LLVertexBuffer::sGLRenderIndices`
- `LLVertexBuffer::sLastMask`

OpenGL families:

- `glBindBuffer`

Current behavior:

- `unbind()` binds both `GL_ARRAY_BUFFER` and `GL_ELEMENT_ARRAY_BUFFER` to 0 and
  clears static trackers
- `setBuffer()` flushes pending mapped data before binding for draw
- `setBuffer()` binds the array buffer when `sGLRenderBuffer` differs from
  `mGLBuffer`
- `setBuffer()` binds the element array buffer when `sGLRenderIndices` differs
  from `mGLIndices`
- `_unmapBuffer()` also binds buffers when flushing dirty mapped regions or
  recreating Apple buffers

Risk level: high.

Reasons:

- static trackers must match actual OpenGL binding state
- attribute pointer setup depends on the correct array buffer binding
- draw calls assert that instance GL names match static trackers
- external raw `glBindBuffer(...)` calls could desynchronize the trackers

## Attribute Array And Layout Ownership

Likely owner functions:

- `LLVertexBuffer::setupClientArrays(...)`
- `LLVertexBuffer::setupVertexBuffer()`
- `LLVertexBuffer::setBuffer()`

OpenGL families:

- `glEnableVertexAttribArray`
- `glDisableVertexAttribArray`
- `glVertexAttribPointer`
- `glVertexAttribIPointer`

Current behavior:

- `setupClientArrays(...)` enables and disables shader attribute arrays based
  on `sLastMask` and the requested data mask
- `setBuffer()` requires a current shader and uses
  `LLGLSLShader::sCurBoundShaderPtr->mAttributeMask`
- `setupVertexBuffer()` maps `LLVertexBuffer::AttributeType` enum positions
  directly to shader attribute locations
- float attributes use `glVertexAttribPointer(...)`
- integer joint and texture index attributes use `glVertexAttribIPointer(...)`
- emissive data can be mapped into the color attribute when color is not also
  bound

Risk level: high.

Reasons:

- enum order must match shader reserved attributes
- pointer offsets depend on `mOffsets` and `sTypeSize`
- attribute setup depends on the currently bound array buffer
- `sLastMask` is shared state across vertex buffers and shaders

## Draw Ownership

Likely owner functions:

- `LLVertexBuffer::drawRange(...)`
- `LLVertexBuffer::drawRangeFast(...)`
- `LLVertexBuffer::draw(...)`
- `LLVertexBuffer::drawArrays(...)`
- static immediate-mode fallback helpers

OpenGL families:

- `glDrawRangeElements`
- `glDrawArrays`

Current behavior:

- `drawRange(...)` validates ranges in debug mode, syncs matrices, and draws
  indexed geometry
- `drawRangeFast(...)` skips validation and matrix sync
- `draw(...)` delegates to `drawRange(...)`
- member `drawArrays(...)` draws non-indexed geometry after tracker asserts
- static helpers use `gGL` immediate-style paths instead of VBO draw calls

Risk level: medium-high.

Reasons:

- draw calls assume `setBuffer()` has already bound the matching buffers
- index offset is converted to a byte pointer using `mIndicesStride`
- debug validation is intentionally not in the fast path

## Ordering Contract

For allocation:

- generate or reuse a GL name before binding it
- bind before `glBufferData(...)`
- update `sGLRenderBuffer` or `sGLRenderIndices` after binding in paths that
  claim the current binding
- keep pool accounting changes aligned with allocation/free operations

For map/unmap and flush:

- `_mapBuffer()` must mark the buffer as mapped and add it to
  `sMappedBuffers`
- mapped vertex/index regions must be merged before flush
- non-Apple flush must bind the matching buffer before `glBufferSubData(...)`
- Apple unmap may recreate GL buffers from CPU-side mapped data
- dirty mapped regions must be cleared after flush

For rendering:

- `setBuffer()` must flush pending mapped data before draw binding
- a shader must be bound before attribute layout setup
- array buffer binding must precede `setupVertexBuffer()`
- index buffer binding must precede indexed draw calls
- `setupClientArrays(...)` and `sLastMask` must stay coherent

## Not A Generic Containment API

Do not move this behavior to `llglcontainment.*` yet.

Reason:

- buffer binding is tied to `LLVertexBuffer` static trackers
- dirty-region flushing is tied to `LLVertexBuffer` mapped memory ownership
- attribute layout depends on `LLVertexBuffer` enum ordering and shader masks
- VBO pooling differs by platform and driver behavior
- a generic wrapper would hide ordering rules that still need local review

## Candidate Source Cleanup

Small naming-only cleanup is applied for GL buffer name generation/deletion and
stays local to:

- `indra/llrender/llvertexbuffer.cpp`

Local helper groups:

- GL buffer name generation/deletion helpers

Not yet applied:

- array/index buffer bind helpers that update static trackers
- buffer storage allocation helper
- buffer sub-data upload helper
- attribute array enable/disable helper
- attribute pointer setup helpers
- draw call helpers

Constraints:

- do not change public headers
- do not change VBO pool policy
- do not change Apple versus non-Apple behavior
- do not change dirty-region merging
- do not change shader attribute enum assumptions
- do not move behavior into `llglcontainment.*`

## Source Cleanup Applied

Applied in phase 2:

- added local helpers for `glGenBuffers(...)` and `glDeleteBuffers(...)`
- replaced buffer name generation callsites in `gen_buffer()`
- replaced the delayed deletion callsite in `delete_buffers(...)`
- kept the AMD one-buffer-at-a-time workaround unchanged
- kept delayed deletion timing unchanged
- kept public headers unchanged
- did not move behavior into `llglcontainment.*`

This does not change ownership: `LLVertexBuffer` still owns buffer name pooling
and delayed deletion.

## Verification

Minimum:

- build `llrender/fast`: passed
- run `git diff --check`: passed

If binding, dirty-region flushing, attribute layout, or draw calls change:

- run the local incremental Xcode arm64 Release build
- launch the viewer
- verify login screen
- load a geometry-heavy scene
- check avatars, terrain, transparent geometry, rigged mesh, and UI geometry
  for missing, scrambled, or wrongly attributed vertices
