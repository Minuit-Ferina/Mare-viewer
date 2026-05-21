# LLRenderTarget FBO Lifetime Contract

This document records the current framebuffer object name lifetime behavior of
`LLRenderTarget`. It is part of the phase 2 containment work.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Inventory

Current local calls in `indra/llrender/llrendertarget.cpp`:

- `glGenFramebuffers(...)`: 2
- `glDeleteFramebuffers(...)`: 1

These calls create and destroy the OpenGL FBO name stored in `mFBO`.

## Ownership

Owner:

- `LLRenderTarget`

Methods involved:

- `allocate()`
- `setColorAttachment()`
- `release()`

The FBO name is owned by the render target instance. Callers do not own `mFBO`
directly and should not delete it.

## Current Ordering

### FBO Generation

Used by:

- `allocate()`
- `setColorAttachment()`

Current behavior:

- `allocate()` releases any existing render target state before generating a
  new FBO name.
- `allocate()` generates an FBO before depth/color attachments are installed.
- `setColorAttachment()` generates an FBO only if `mFBO` is currently 0.

Contract:

- A render target FBO name must exist before texture attachments are mutated.
- Generating an FBO name must not update `sCurFBO`.
- Generating an FBO name must not push or pop `sBoundTarget`.

### FBO Deletion

Used by:

- `release()`

Current behavior:

- Owned depth texture state is released before the FBO name is deleted.
- Shared depth and extra color attachments are detached before the FBO name is
  deleted.
- If `mFBO` is still tracked as `sCurFBO`, framebuffer 0 is rebound and
  `sCurFBO` is cleared before deleting the FBO name.
- `mFBO` is set to 0 after deletion.

Contract:

- Deleting an FBO must not leave `sCurFBO` pointing at the deleted name.
- Deleting an FBO must not happen while the render target is in the bound
  target stack.
- The local `mFBO` field is the source of truth for whether this instance owns
  an FBO name.

## Risk

Risk level: high.

Reasons:

- FBO names are OpenGL object lifetime state.
- Deleting a still-tracked FBO can leak invalid global GL state into later
  rendering.
- Generating an FBO too early or too often can leak names.
- `swapFBORefs()` can exchange FBO ownership between two render targets, so
  release logic must keep using the current instance field.

## Not A Generic Containment API

Do not add `LLGLContainment` FBO lifetime helpers yet.

Reason:

- `mFBO` ownership is local to `LLRenderTarget`.
- The object lifetime sequence is tied to texture attachment state and
  `sCurFBO`.
- A global helper would not know whether the owner must clear local fields,
  detach attachments, or reset tracked framebuffer state.

## Source Cleanup Boundary

The safe source cleanup is naming-only and stays inside:

- `indra/llrender/llrendertarget.cpp`

Constraints:

- Do not change public headers.
- Do not touch `pipeline.cpp`.
- Do not change texture attachment or deletion order.
- Do not change `sCurFBO` or `sBoundTarget` semantics.
- Do not add behavior to `llglcontainment.*`.

## Source Cleanup Applied

The phase 2 source cleanup names the existing FBO lifetime intents locally:

- generate a framebuffer object name
- delete a framebuffer object name

Implementation shape:

- added internal helper `generate_framebuffer_name(...)`
- added internal helper `delete_framebuffer_name(...)`
- kept both helpers in `llrendertarget.cpp`
- replaced direct `glGenFramebuffers(...)` and `glDeleteFramebuffers(...)`
  callsites in `llrendertarget.cpp`
- did not change public headers
- did not add an `LLGLContainment` FBO lifetime wrapper

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
