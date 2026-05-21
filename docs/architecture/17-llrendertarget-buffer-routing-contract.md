# LLRenderTarget Buffer Routing Contract

This document records the current draw/read buffer routing behavior of
`LLRenderTarget`. It is documentation for the phase 2 containment work and does
not change source behavior by itself.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Inventory

Current local calls in `indra/llrender/llrendertarget.cpp`:

- `glDrawBuffer(...)`: 2
- `glReadBuffer(...)`: 2
- `glDrawBuffers(...)`: 1

## State Being Routed

OpenGL draw/read buffer routing decides where fragment output and readback
operations go after an FBO is bound.

In `LLRenderTarget`, this routing is tied to two existing states:

- the currently bound render target FBO
- whether that render target has one or more color attachments

This routing is not texture ownership, FBO ownership, or viewport ownership.
Those remain separate contracts.

## Render Target Routing

Owner:

- `LLRenderTarget::bindTarget()`

Current behavior:

- If the target has no color textures, draw and read buffers are set to
  `GL_NONE`.
- If the target has color textures, draw buffers are set to
  `GL_COLOR_ATTACHMENT0` through the active attachment count, and the read
  buffer is set to `GL_COLOR_ATTACHMENT0`.

Contract:

- Render target buffer routing happens after binding the render target FBO.
- The selected draw buffer count must match the current color attachment count.
- A depth-only target must not route color output to an attachment.
- This routing must remain paired with the `bindTarget()` / `flush()` scope.

## Default Framebuffer Restore

Owner:

- `LLRenderTarget::flush()` when there is no previous render target to restore

Current behavior:

- `flush()` returns to framebuffer 0.
- It restores the default framebuffer viewport from `gGLViewport`.
- It restores read and draw buffers to `GL_BACK`.

Contract:

- Returning to the default framebuffer must restore back-buffer routing.
- This is separate from FBO binding and viewport restore, even though the calls
  happen together in `flush()`.

## Risk

Risk level: medium-high.

Reasons:

- Draw/read buffers are global OpenGL state.
- Bad routing can silently render into no color buffer or the wrong attachment.
- Depth-only render targets intentionally route color output to `GL_NONE`.
- The render target stack can nest, so routing must follow the existing
  `bindTarget()` / `flush()` ordering.

## Not A Generic Containment API

Do not add a generic `LLGLContainment::drawBuffer(...)` or
`LLGLContainment::readBuffer(...)` helper yet.

Reason:

- The local owner is still `LLRenderTarget`.
- These calls only make sense after the local FBO binding and attachment count
  are known.
- A generic wrapper would hide whether the code is configuring a render target,
  restoring the default framebuffer, or doing unrelated readback work.

## Source Cleanup Boundary

The safe source cleanup is naming-only and stays inside:

- `indra/llrender/llrendertarget.cpp`

Constraints:

- Do not change public headers.
- Do not touch `pipeline.cpp`.
- Do not add behavior to `llglcontainment.*`.
- Do not change attachment counts or target formats.
- Do not change the `bindTarget()` / `flush()` pairing.

## Source Cleanup Applied

The phase 2 source cleanup names the two existing intents locally:

- configure draw/read buffers for the currently bound render target
- restore draw/read buffers for the default framebuffer

Implementation shape:

- added internal helper `set_render_target_buffer_routing(...)`
- added internal helper `restore_default_framebuffer_buffer_routing()`
- kept both helpers in `llrendertarget.cpp`
- changed `bindTarget()` to call the render target routing helper
- changed `flush()` to call the default framebuffer routing helper
- did not change public headers
- did not add an `LLGLContainment` routing wrapper

## Verification

Minimum verification for the applied source cleanup:

- build `llrender/fast`
- run `git diff --check`

If future changes broaden this beyond `llrendertarget.cpp`, also run the local
incremental Xcode arm64 Release build and a runtime smoke test.

## Source Cleanup Build Check

Date: 2026-05-21 CEST

Configured build tree:

```text
/private/tmp/Mare-viewer-phase2-llrender-make3
```

Targeted build:

```sh
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 \
  --target llrender/fast -- -j8
```

Result: passed.

Observed work:

- compiled `llrender/CMakeFiles/llrender.dir/llrendertarget.cpp.o`
- relinked `libllrender.a`

Build-tree note:

- the temporary Makefile tree reused the already installed Xcode build
  `packages` directory to avoid reinstalling prebuilts for this narrow check
- no clean viewer build was run
