# LLRenderTarget FBO Contract

This document records the current framebuffer binding behavior of
`LLRenderTarget`. It is documentation only and does not change source behavior.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Inventory

Current local counts in `indra/llrender/llrendertarget.cpp`:

- `glBindFramebuffer(...)`: 17
- `glFramebufferTexture2D(...)`: 7
- `LLRenderTarget::sCurFBO`: 15
- `LLRenderTarget::sBoundTarget`: 7

## State Variables

`LLRenderTarget` tracks two different concepts:

- `sCurFBO`: the framebuffer that the render target layer considers current
  for drawing.
- `sBoundTarget`: the top of the render target stack used by
  `bindTarget()` / `flush()`.

These are not the same as arbitrary temporary OpenGL framebuffer binds used
while attaching or detaching textures.

## Binding Intents

### Render Target Binding

Owner:

- `LLRenderTarget::bindTarget()`
- `LLRenderTarget::flush()`

Current behavior:

- `bindTarget()` binds `mFBO`, updates `sCurFBO`, configures read/draw buffers,
  checks framebuffer status, applies the render target viewport, and pushes the
  previous target through `mPreviousRT`.
- `flush()` asserts that the target is current, generates mipmaps if requested,
  then either rebinds the previous render target or returns to framebuffer 0.

Contract:

- This path owns draw-time FBO binding.
- This path owns `sCurFBO` updates.
- This path owns `sBoundTarget` stack updates.
- Callers must pair one `bindTarget()` with one `flush()`.

### Attachment Mutation Binding

Owner:

- `LLRenderTarget::allocate()`
- `LLRenderTarget::setColorAttachment()`
- `LLRenderTarget::releaseColorAttachment()`
- `LLRenderTarget::addColorAttachment()`
- `LLRenderTarget::shareDepthBuffer()`
- `LLRenderTarget::release()`

Current behavior:

- These methods temporarily bind an FBO to attach or detach depth/color
  textures.
- They restore the OpenGL framebuffer binding with
  `glBindFramebuffer(GL_FRAMEBUFFER, sCurFBO)`.
- They generally do not update `sCurFBO` when temporarily binding the target
  being modified.

Contract:

- Attachment mutation binding is not a render target stack operation.
- It must not update `sBoundTarget`.
- It must not treat the temporary FBO as the current draw target.
- It must restore the OpenGL binding to `sCurFBO` before returning.

### Default Framebuffer Restore

Owner:

- `LLRenderTarget::flush()`
- `LLRenderTarget::release()` when deleting the currently tracked FBO

Current behavior:

- `flush()` binds framebuffer 0, sets `sCurFBO` to 0, restores the default
  framebuffer viewport, and restores back-buffer read/draw buffers.
- `release()` defensively resets `sCurFBO` and binds framebuffer 0 if the FBO
  being deleted is still tracked as current.

Contract:

- Returning to framebuffer 0 must keep `sCurFBO` in sync.
- Returning to framebuffer 0 from `flush()` must also restore the viewport.
- Deleting an FBO must not leave `sCurFBO` pointing at a deleted name.

## Risk

Risk level: high.

Reasons:

- Framebuffer binding is global OpenGL state.
- `LLRenderTarget` mixes draw-time binding with temporary attachment mutation
  binding.
- Updating `sCurFBO` in the wrong path could break attachment edits or nested
  render target restoration.
- Failing to restore OpenGL binding after attachment mutation can leak a
  temporary FBO into later rendering.
- `pipeline.cpp`, dynamic textures, probes, impostors, terrain bakes, and UI
  preview paths all depend on this behavior indirectly.

## Not A First Generic Containment API

Do not add a generic `LLGLContainment::bindFramebuffer(...)` wrapper yet.

Reason:

- The same OpenGL call has at least two local meanings here: draw-time target
  binding and temporary attachment mutation.
- A generic wrapper would hide the ownership distinction unless its API names
  the intent.
- The current owner is still `LLRenderTarget`, not a global containment layer.

## Candidate Future Source Cleanup

A future source patch may be reasonable if it is naming-only and remains inside
`indra/llrender/llrendertarget.cpp`.

Possible helper names:

- `bind_render_target_fbo(...)`
- `restore_tracked_fbo_binding()`
- `temporarily_bind_attachment_fbo(...)`

Constraints:

- Do not change public headers.
- Do not move code into `llglcontainment.*`.
- Do not touch `pipeline.cpp` in the same patch.
- Do not update `sCurFBO` from attachment mutation helpers.
- Do not change the `bindTarget()` / `flush()` stack contract.

## Verification For A Future Source Patch

Minimum:

- build `llrender/fast`
- run `git diff --check`

If any behavior changes or any file outside `indra/llrender/llrendertarget.cpp`
is touched:

- run the local Xcode arm64 Release build
- launch the viewer
- verify login screen
- verify 2D UI and 3D world alignment after window resize
- run the empty-area and loaded-area FPS baseline scenes

## Decision

The next source-side FBO cleanup should only name existing local intent. It
should not introduce a broader OpenGL containment layer.
