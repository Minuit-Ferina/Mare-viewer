# LLRenderTarget Buffer Routing Containment Task

This document defines the next phase 3 source task before editing
`llglcontainment.*` again.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Related contracts:

- `docs/architecture/17-llrendertarget-buffer-routing-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/42-phase3-rendertarget-combined-review.md`

## Task

Move only the raw draw/read buffer routing OpenGL calls behind
`llglcontainment.*`, while keeping `LLRenderTarget` as the owner of render
target routing intent and ordering.

This continues the phase 3 work inside the same owner.

## Candidate API

Add the narrowest possible functions:

- `LLGLContainment::setDrawBuffer(...)`
- `LLGLContainment::setReadBuffer(...)`
- `LLGLContainment::setDrawBuffers(...)`

Intended raw operations:

- `glDrawBuffer(...)`
- `glReadBuffer(...)`
- `glDrawBuffers(...)`

Reason for this shape:

- current `LLRenderTarget` routing helpers already express whether the code is
  configuring a render target or restoring the default framebuffer
- `LLRenderTarget` still decides attachment count, depth-only `GL_NONE`
  routing, and default `GL_BACK` restore
- `LLGLContainment` only issues the raw OpenGL calls

## Ownership Boundary

`llglcontainment.*` may own:

- the raw draw buffer call
- the raw read buffer call
- the raw draw buffers call

`LLRenderTarget` must still own:

- whether the current target is depth-only
- color attachment count
- construction of the `GL_COLOR_ATTACHMENT*` routing array
- read buffer selection for render targets
- default framebuffer `GL_BACK` restore policy
- ordering relative to FBO binding, viewport restore, and `flush()`

## Source Patch Shape

Only these local helper bodies should change in the source patch:

- `set_render_target_buffer_routing(...)`
- `restore_default_framebuffer_buffer_routing()`

Expected delegation:

- local routing helpers call the new `LLGLContainment` functions
- local routing helpers keep all `GL_NONE`, `GL_COLOR_ATTACHMENT*`, and
  `GL_BACK` decisions

No public `LLRenderTarget` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- attachment count decisions
- depth-only routing behavior
- `GL_COLOR_ATTACHMENT*` array construction
- default framebuffer restore policy
- `bindTarget()` / `flush()` pairing
- FBO binding
- viewport restore

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL calls are issued, not which buffers are
selected or when routing is applied.

## Review Checklist

Reviewers should check:

- `LLGLContainment` gains only draw/read buffer raw-call helpers
- render target routing still sends depth-only targets to `GL_NONE`
- color target routing still uses `GL_COLOR_ATTACHMENT0...N`
- render target read buffer still uses `GL_COLOR_ATTACHMENT0`
- default framebuffer restore still uses `GL_BACK`
- routing is still applied after render target FBO binding
- default routing is still restored only when flushing to framebuffer 0
- no draw-pool, `pipeline.cpp`, UI, shader, or texture upload code is touched

## Verification Plan

Minimum source verification:

- build `llrender/fast`
- run `git diff --check`
- regenerate generated source inventory

Because this is another behavior-bearing `llglcontainment.*` packet, also run:

- local incremental Xcode arm64 Release build
- executable architecture check
- runtime dylib presence check

Loaded-scene runtime smoke remains recommended before widening phase 3 beyond
`LLRenderTarget`.
