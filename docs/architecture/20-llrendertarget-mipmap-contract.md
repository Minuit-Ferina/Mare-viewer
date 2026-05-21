# LLRenderTarget Mipmap Contract

This document records the current render target mipmap generation behavior.
It is part of the phase 2 containment work.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Inventory

Current local calls in `indra/llrender/llrendertarget.cpp`:

- `glGenerateMipmap(...)`: 1

## Ownership

Owner:

- `LLRenderTarget::flush()`

The mipmap operation belongs to the render target scope. It is not a generic
texture upload path and it does not own texture lifetime.

## Current Ordering

Current behavior:

1. `flush()` verifies this render target is the current FBO and bound target.
2. If `mGenerateMipMaps == LLTexUnit::TMG_AUTO`, attachment 0 is bound on
   texture channel 0 with trilinear filtering.
3. `glGenerateMipmap(GL_TEXTURE_2D)` is called.
4. `flush()` restores the previous render target or the default framebuffer.

Contract:

- Mipmap generation happens before leaving the render target scope.
- It only runs for automatic mipmap generation.
- It assumes color attachment 0 is the texture whose mipmaps should be
  generated.
- It must not change the `bindTarget()` / `flush()` stack contract.

## Risk

Risk level: medium.

Reasons:

- Mipmap generation depends on texture binding state.
- The current call hard-codes `GL_TEXTURE_2D`.
- Generating mipmaps at the wrong time could sample stale render target output.
- This is tied to `mGenerateMipMaps`, not to all render target textures.

## Not A Generic Containment API

Do not add an `LLGLContainment` mipmap helper yet.

Reason:

- The current owner is still `LLRenderTarget::flush()`.
- The operation depends on render target completion and attachment 0 binding.
- A generic helper would hide the local ordering requirement.

## Source Cleanup Boundary

The safe source cleanup is naming-only and stays inside:

- `indra/llrender/llrendertarget.cpp`

Constraints:

- Do not change public headers.
- Do not touch texture upload code.
- Do not touch `pipeline.cpp`.
- Do not change which texture or target receives mipmaps.
- Do not add behavior to `llglcontainment.*`.

## Source Cleanup Applied

The phase 2 source cleanup names the existing intent locally:

- generate mipmaps for the currently bound render target color texture

Implementation shape:

- added internal helper `generate_bound_render_target_mipmaps()`
- kept the helper in `llrendertarget.cpp`
- replaced the direct `glGenerateMipmap(...)` callsite in `flush()`
- did not change public headers
- did not add an `LLGLContainment` mipmap wrapper

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
