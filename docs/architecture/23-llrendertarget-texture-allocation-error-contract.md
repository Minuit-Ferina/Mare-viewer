# LLRenderTarget Texture Allocation Error Contract

This document records the current texture allocation error checks in
`LLRenderTarget`. It is part of the phase 2 containment work.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Inventory

Current local calls in `indra/llrender/llrendertarget.cpp`:

- `glGetError(...)`: 2

Both calls are used after render target texture allocation attempts.

## Ownership

Owner:

- `LLRenderTarget`

Methods involved:

- `addColorAttachment()`
- `allocateDepth()`

These error checks belong to render target texture allocation. They are not a
general OpenGL error policy and they do not own global debug behavior.

## Current Behavior

### Color Attachment Allocation

Current behavior:

1. A color texture name is generated and bound.
2. `clear_glerror()` clears any prior GL error.
3. `LLImageGL::setManualImage(...)` allocates the color image.
4. `glGetError()` is checked.
5. Any GL error logs a warning and returns `false`.

### Depth Attachment Allocation

Current behavior:

1. A depth texture name is generated and bound.
2. `clear_glerror()` clears any prior GL error.
3. `LLImageGL::setManualImage(...)` allocates the depth image.
4. Filtering is set to point filtering.
5. The allocation byte counter is incremented.
6. `glGetError()` is checked.
7. Any GL error logs a warning and returns `false`.

Contract:

- These checks must remain after the existing `clear_glerror()` calls.
- These checks consume the current OpenGL error state.
- They must not bind textures, bind FBOs, or mutate render target stack state.
- They must not become a global replacement for `stop_glerror()`.

## Risk

Risk level: medium.

Reasons:

- `glGetError()` consumes the GL error state.
- Moving the check can change which operation is reported as failed.
- These checks affect whether allocation returns success or failure.
- The depth path currently checks after setting filtering and incrementing
  tracked bytes; this ordering is existing behavior and must remain unchanged
  in a naming-only cleanup.

## Not A Generic Containment API

Do not add an `LLGLContainment` error helper yet.

Reason:

- Error policy differs across existing callsites.
- These checks are allocation-specific and return boolean failure to the local
  caller.
- A global helper would hide the difference between debug assertions,
  allocation failure checks, and broad `stop_glerror()` usage.

## Source Cleanup Boundary

The safe source cleanup is naming-only and stays inside:

- `indra/llrender/llrendertarget.cpp`

Constraints:

- Do not change public headers.
- Do not touch `LLImageGL`.
- Do not change `clear_glerror()` / `stop_glerror()` placement.
- Do not change allocation return behavior.
- Do not add behavior to `llglcontainment.*`.

## Source Cleanup Applied

The phase 2 source cleanup names the existing allocation error intent locally:

- check whether the current render target texture allocation has reported a GL
  error

Implementation shape:

- added internal helper `render_target_texture_allocation_failed()`
- kept the helper in `llrendertarget.cpp`
- replaced both direct `glGetError()` callsites in `llrendertarget.cpp`
- did not change public headers
- did not add an `LLGLContainment` error wrapper

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
