# LLRenderTarget Framebuffer Status Contract

This document records the current framebuffer completeness check behavior in
`LLRenderTarget`. It is part of the phase 2 containment work.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Inventory

Current local calls in `indra/llrender/llrendertarget.cpp`:

- `glCheckFramebufferStatus(...)`: 1

The call checks `GL_DRAW_FRAMEBUFFER`.

## Ownership

Owner:

- `LLRenderTarget`

Methods using the check:

- `setColorAttachment()`
- `addColorAttachment()`
- `shareDepthBuffer()`
- `bindTarget()`
- `clear()`

The check belongs to render target mutation and binding. It is not a general
OpenGL debug layer.

## Current Behavior

Current behavior:

- when `gDebugGL` is false, no status check is performed
- when `gDebugGL` is true, `GL_DRAW_FRAMEBUFFER` is checked
- `GL_FRAMEBUFFER_COMPLETE` passes
- any other status logs a warning and calls `ll_fail(...)`

Contract:

- The check must only run in the existing debug path.
- The check must describe the currently bound draw framebuffer.
- The check must not bind, restore, or mutate FBO state.
- Callers remain responsible for binding the correct FBO before checking.

## Risk

Risk level: medium-high.

Reasons:

- Framebuffer completeness depends on current global FBO binding.
- Checking the wrong target would give misleading debug failures.
- Adding unconditional checks could change performance or failure behavior.
- Mutating binding in the status helper would break attachment and render
  target stack ordering.

## Not A Generic Containment API

Do not add an `LLGLContainment` status helper yet.

Reason:

- The check is still a local debug assertion for `LLRenderTarget`.
- A global helper would need a broader error/reporting policy.
- Platform or debug-layer policy should not be introduced as part of this
  naming-only pass.

## Source Cleanup Boundary

The safe source cleanup is naming-only and stays inside:

- `indra/llrender/llrendertarget.cpp`

Constraints:

- Do not change public headers.
- Do not touch `pipeline.cpp`.
- Do not change `gDebugGL` behavior.
- Do not change FBO binding order.
- Do not add behavior to `llglcontainment.*`.

## Source Cleanup Applied

The phase 2 source cleanup names the existing status intent locally:

- check the current draw framebuffer status

Implementation shape:

- renamed local helper `check_framebuffer_status()` to
  `check_current_draw_framebuffer_status()`
- moved it into the existing anonymous namespace with the other local helpers
- replaced local callsites in `llrendertarget.cpp`
- did not change public headers
- did not add an `LLGLContainment` status wrapper

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

No clean viewer build was run.
