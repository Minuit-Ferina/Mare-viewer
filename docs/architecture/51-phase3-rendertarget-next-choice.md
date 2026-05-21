# Phase 3 Render Target Next Choice

This document chooses the next `LLRenderTarget` containment packet after the
completed mipmap packet.

Branch: `phase3`

Base branch: `phase2`

## Remaining Direct Calls

Current direct OpenGL calls in `indra/llrender/llrendertarget.cpp`:

- `glViewport(...)`: 2
- `glClear(...)`: 1
- `glScissor(...)`: 1
- `glGetError(...)`: 1

## Candidate Review

### Viewport

Decision: defer.

Reason:

- default framebuffer restore depends on external `gGLViewport`
- mistakes can affect UI/world alignment and resize behavior
- verification should include a fresh runtime resize and loaded-scene check

### Texture Allocation Error Check

Decision: defer.

Reason:

- `glGetError()` consumes OpenGL error state
- moving it is small, but the behavior is allocation-result policy rather than
  render output state
- it is better handled after visible render target state changes are complete

### Clear And Scissor

Decision: choose next.

Reason:

- both calls belong to the same `LLRenderTarget::clear()` owner contract
- both calls are already isolated behind local phase 2 helpers
- `LLRenderTarget` can keep mask computation, FBO status checking, scissor
  enable scope, and error-check placement local
- the source patch can be limited to two helper bodies

## Next Task

Add a task-specific containment note for render target clear/scissor before
source edits.

The next source task should move only these raw calls:

- `glClear(...)`
- `glScissor(...)`

It should not move:

- clear mask calculation
- `mUseDepth` policy
- `mFBO` branch policy
- framebuffer status checking
- `LLGLEnable scissor(GL_SCISSOR_TEST)`
- `stop_glerror()` placement
