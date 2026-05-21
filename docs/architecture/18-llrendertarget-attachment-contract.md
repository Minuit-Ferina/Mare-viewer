# LLRenderTarget Attachment Contract

This document records the current FBO texture attachment behavior of
`LLRenderTarget`. It is part of the phase 2 containment work.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Inventory

Current local calls in `indra/llrender/llrendertarget.cpp`:

- `glFramebufferTexture2D(...)`: 7

These calls attach or detach texture names from the FBO currently bound for
attachment mutation.

## Ownership

Owner:

- `LLRenderTarget`

Methods involved:

- `allocate()`
- `setColorAttachment()`
- `releaseColorAttachment()`
- `addColorAttachment()`
- `shareDepthBuffer()`
- `release()`

The attachment operations are local to render target allocation, mutation,
sharing, and release. They do not define render pass order and they do not own
viewer texture lifetime outside the render target's own `mTex` and `mDepth`
state.

## Current Ordering

The current ordering is:

1. Bind the target FBO for attachment mutation.
2. Attach or detach one depth or color texture.
3. Optionally check framebuffer status.
4. Restore the tracked FBO binding.

This is not the same as draw-time target binding. Attachment mutation must not
update `sCurFBO` and must not push or pop `sBoundTarget`.

## Attachment Intents

### Depth Attach

Used by:

- `allocate()`
- `shareDepthBuffer()`

Current behavior:

- attach `mDepth` to `GL_DEPTH_ATTACHMENT`
- use `LLTexUnit::getInternalType(mUsage)` as the texture target
- use mip level 0

### Color Attach

Used by:

- `setColorAttachment()`
- `addColorAttachment()`

Current behavior:

- attach the color texture name to `GL_COLOR_ATTACHMENT0` plus the attachment
  index
- use `LLTexUnit::getInternalType(mUsage)` as the texture target
- use mip level 0

### Depth Detach

Used by:

- `release()` when detaching a shared depth buffer

Current behavior:

- attach texture name 0 to `GL_DEPTH_ATTACHMENT`
- use the same texture target expression as attachment
- set `mUseDepth` false after detaching

### Color Detach

Used by:

- `releaseColorAttachment()`
- `release()` when detaching extra color attachments

Current behavior:

- attach texture name 0 to the color attachment
- delete owned extra color textures only after detaching them

## Risk

Risk level: high.

Reasons:

- Texture attachment is part of FBO completeness.
- Incorrect attachment target can make the render target incomplete.
- Detaching in the wrong order can leave an FBO referencing a deleted texture.
- Shared depth buffers use another render target's FBO but this target's depth
  texture.
- `pipeline.cpp`, probes, impostors, dynamic textures, and UI previews can all
  consume render targets that depend on this behavior.

## Not A Generic Containment API

Do not add `LLGLContainment` helpers for these calls yet.

Reason:

- The local owner is still `LLRenderTarget`.
- The operation depends on `mUsage`, attachment index, and texture ownership.
- A generic wrapper would hide the distinction between attach and detach.

## Source Cleanup Boundary

The safe source cleanup is naming-only and stays inside:

- `indra/llrender/llrendertarget.cpp`

Constraints:

- Do not change public headers.
- Do not touch `pipeline.cpp`.
- Do not change texture generation or deletion.
- Do not change FBO binding order.
- Do not change `sCurFBO` or `sBoundTarget` updates.
- Do not add behavior to `llglcontainment.*`.

## Source Cleanup Applied

The phase 2 source cleanup names the existing attachment intents locally:

- set a framebuffer texture attachment
- clear a framebuffer texture attachment

Implementation shape:

- added internal helper `set_framebuffer_texture_attachment(...)`
- added internal helper `clear_framebuffer_texture_attachment(...)`
- kept both helpers in `llrendertarget.cpp`
- replaced direct `glFramebufferTexture2D(...)` callsites in
  `llrendertarget.cpp`
- did not change public headers
- did not add an `LLGLContainment` attachment wrapper

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
