# LLRenderTarget Viewport Contract

This document records the current viewport behavior of `LLRenderTarget`.
It is a contract note only and does not change source behavior.

Related file:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Current Public Contract

The header already describes the core usage pattern:

```cpp
target.bindTarget();
// issue drawing commands
target.flush();
```

The viewport contract is part of that pairing.

## Observed State

Static render target state:

- `LLRenderTarget::sCurFBO`
- `LLRenderTarget::sCurResX`
- `LLRenderTarget::sCurResY`
- `LLRenderTarget::sBoundTarget`

External viewer viewport state:

- `gGLViewport`

Important distinction:

- `sCurResX` and `sCurResY` describe the current render target resolution.
- `gGLViewport` describes the canonical viewer viewport used when returning to
  the default framebuffer.

`LLRenderTarget` should not treat `gGLViewport` as a copy of every temporary
render target viewport.

## bindTarget Contract

`LLRenderTarget::bindTarget()` currently:

- requires `mFBO` to be valid
- asserts that the target is not already bound in the render target stack
- binds `mFBO`
- configures draw and read buffers
- sets OpenGL viewport to `(0, 0, mResX, mResY)`
- updates `sCurResX` and `sCurResY`
- stores the previous bound target in `mPreviousRT`
- makes this target `sBoundTarget`

Contract:

- `bindTarget()` owns the temporary render target viewport for this target.
- `bindTarget()` must not update `gGLViewport`.
- `bindTarget()` assumes the caller will later call `flush()` exactly once.

## flush Contract

`LLRenderTarget::flush()` currently:

- flushes pending `gGL` work
- asserts this target is the current FBO and current bound target
- optionally generates mipmaps
- if a previous render target exists, rebinds that previous target
- otherwise returns to the default framebuffer and restores viewport from
  `gGLViewport`
- updates `sCurResX` and `sCurResY` after restoring the default framebuffer
- restores default read and draw buffers

Contract:

- `flush()` completes the temporary viewport scope started by `bindTarget()`.
- If nested inside another render target, `flush()` restores the previous
  render target by rebinding it.
- If returning to the default framebuffer, `flush()` restores the viewer
  viewport from `gGLViewport`.
- `flush()` must not recompute the viewer viewport from window state.

## getViewport Contract

`LLRenderTarget::getViewport(S32* viewport)` currently returns:

- `x = 0`
- `y = 0`
- `width = mResX`
- `height = mResY`

Contract:

- The returned viewport describes the target-local viewport.
- It does not describe the viewer window viewport.

## Caller Requirements

Callers must:

- pair `bindTarget()` and `flush()` one-to-one
- avoid leaving a target bound across unrelated rendering code
- ensure `gGLViewport` is current before relying on `flush()` to restore the
  default framebuffer viewport
- use viewer-window or camera code to update `gGLViewport`, not
  `LLRenderTarget`

## Non-Guarantees

`LLRenderTarget` does not currently guarantee:

- arbitrary save/restore of the previous raw OpenGL viewport
- ownership of `gGLViewport`
- ownership of 2D versus 3D viewer viewport selection
- restore of matrices, scissor state, color masks, depth state, shader state,
  or texture bindings beyond the existing behavior

Do not broaden the viewport contract to include those states without a separate
task.

## Source Patch Boundary

If a source cleanup is requested later, keep the first patch inside:

- `indra/llrender/llrendertarget.cpp`

The safest first cleanup would be internal naming only, for example private
helpers that make these two intents explicit:

- apply this render target's viewport
- restore the default framebuffer viewport from `gGLViewport`

Do not introduce a public `LLGLContainment::viewport(...)` wrapper for this.
The existing local owner is `LLRenderTarget`.

Do not change these files in the same patch:

- `indra/newview/pipeline.cpp`
- `indra/newview/llviewerwindow.cpp`
- `indra/newview/llviewercamera.cpp`
- probe managers
- UI preview paths

## Verification For A Future Source Patch

Minimum:

- build `llrender/fast`
- run `git diff --check`

If behavior changes or non-`llrender` files are touched:

- run the local macOS arm64 Release build
- launch the viewer
- verify 2D UI and 3D world alignment after window resize
- run the empty-area FPS baseline scene
- run the loaded-area FPS baseline scene

## Decision

The next source-side step should not add a new abstraction layer. It should
either leave behavior unchanged or make the existing `LLRenderTarget` viewport
intent easier to review locally.
