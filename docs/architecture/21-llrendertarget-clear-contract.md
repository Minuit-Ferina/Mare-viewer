# LLRenderTarget Clear Contract

This document records the current render target clear behavior. It is part of
the phase 2 containment work.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Inventory

Current local calls in `indra/llrender/llrendertarget.cpp`:

- `glClear(...)`: 2
- `glScissor(...)`: 1

## Ownership

Owner:

- `LLRenderTarget::clear()`

This clear path belongs to the render target instance. It is not the global
frame clear path and it does not own clear color, stencil state, or frame
orchestration.

## Current Behavior

`clear(mask_in)` currently:

- starts with `GL_COLOR_BUFFER_BIT`
- adds `GL_DEPTH_BUFFER_BIT` when `mUseDepth` is true
- intersects that render target mask with the caller-provided `mask_in`
- if `mFBO` is valid, checks framebuffer status and clears the selected buffers
- otherwise enables scissor, restricts scissor to the render target dimensions,
  and clears the selected buffers

Contract:

- The clear mask is still owned by `LLRenderTarget::clear()`.
- The clear operation must not alter the `bindTarget()` / `flush()` stack.
- The fallback scissor path must remain target-local.
- This contract does not include clear color or depth clear value ownership.

## Risk

Risk level: medium.

Reasons:

- `glClear` mutates the currently bound framebuffer contents.
- Incorrect mask handling can clear depth or color unexpectedly.
- The fallback scissor path depends on target dimensions and global scissor
  state.
- Clear behavior is visible to render targets used by world, probe, dynamic
  texture, and UI preview paths.

## Not A Generic Containment API

Do not add `LLGLContainment` clear helpers yet.

Reason:

- The local owner is still `LLRenderTarget::clear()`.
- Other `glClear` callsites have different owners: frame orchestration, UI
  preview rendering, platform window glue, and material preview paths.
- A generic clear wrapper would hide the owner-specific mask and state contract.

## Source Cleanup Boundary

The safe source cleanup is naming-only and stays inside:

- `indra/llrender/llrendertarget.cpp`

Constraints:

- Do not change public headers.
- Do not touch global frame clear code.
- Do not touch `pipeline.cpp`.
- Do not change clear masks.
- Do not add behavior to `llglcontainment.*`.

## Source Cleanup Applied

The phase 2 source cleanup names the existing clear intents locally:

- clear the currently selected render target buffers
- set scissor to the render target rectangle for the fallback path

Implementation shape:

- added internal helper `clear_render_target_buffers(...)`
- added internal helper `set_render_target_scissor(...)`
- kept both helpers in `llrendertarget.cpp`
- replaced direct `glClear(...)` and `glScissor(...)` callsites in
  `LLRenderTarget::clear()`
- did not change public headers
- did not add an `LLGLContainment` clear wrapper

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
