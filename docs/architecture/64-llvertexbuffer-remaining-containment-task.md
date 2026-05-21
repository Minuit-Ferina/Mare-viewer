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

## Source Patch Applied

Applied in phase 3:

- added `LLGLContainment::bindBufferObject(...)`
- added `LLGLContainment::allocateBufferObjectStorage(...)`
- added `LLGLContainment::updateBufferObjectSubData(...)`
- added `LLGLContainment::enableVertexAttributeArray(...)`
- added `LLGLContainment::disableVertexAttributeArray(...)`
- added `LLGLContainment::setVertexAttributePointer(...)`
- added `LLGLContainment::setIntegerVertexAttributePointer(...)`
- added `LLGLContainment::drawVertexBufferRange(...)`
- added `LLGLContainment::drawVertexBufferArrays(...)`
- delegated only the matching local `LLVertexBuffer` helper bodies
- kept all static tracker updates, dirty-region logic, shader attribute
  decisions, draw validation, matrix sync, and `STOP_GLERROR` placement in
  `LLVertexBuffer`
- did not touch disabled GL work queue sync code, draw pools, UI rendering,
  shader managers, texture upload code, or `pipeline.cpp`

Generated inventory was regenerated after the source patch so
`docs/architecture/generated/source_inventory.csv` and
`docs/architecture/generated/source_inventory_top.md` reflect the new call
locations.

After this source patch:

- active local `LLVertexBuffer` wrapper helper bodies no longer issue raw
  OpenGL calls directly
- `indra/llrender/llvertexbuffer.cpp` has 2 likely direct `gl*` calls in the
  generated inventory, both from disabled/commented GL work queue sync context
- `indra/llrender/llglcontainment.cpp` has 24 likely direct `gl*` calls in the
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

The phase 3 remaining vertex buffer containment packet was also verified with
the local incremental Xcode arm64 Release build.

Result:

- `xcodebuild -quiet` exited with code 0
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` present in the app bundle

Detailed command and output notes are recorded in:

- `docs/architecture/local-darwin-arm64-build.md`

Runtime smoke is deferred for now by project decision on this wrapper-only
packet family.
