# LLRenderTarget Attachment Containment Task

This document defines the next phase 3 source task before editing
`llglcontainment.*` again.

Related files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Related contracts:

- `docs/architecture/18-llrendertarget-attachment-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/39-phase3-fbo-containment-review.md`

## Task

Move only the raw framebuffer texture attachment OpenGL call behind
`llglcontainment.*`, while keeping `LLRenderTarget` as the owner of attachment
intent, texture ownership, and ordering.

This follows the first phase 3 FBO bind/status packet and stays within the
same owner.

## Candidate API

Add one narrow function:

- `LLGLContainment::setReadWriteFramebufferTexture2D(...)`

Intended raw operation:

- `glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture_target,
  texture_name, mip_level)`

Reason for this shape:

- current `LLRenderTarget` attachment helpers operate on `GL_FRAMEBUFFER`
- the local owner still chooses color versus depth attachment
- the local owner still converts `mUsage` through `LLTexUnit::getInternalType`
- the local owner still passes mip level 0
- texture name 0 remains the local detach convention

## Ownership Boundary

`llglcontainment.*` may own:

- the raw `glFramebufferTexture2D(GL_FRAMEBUFFER, ...)` call

`LLRenderTarget` must still own:

- attachment selection: depth versus color
- attachment index math
- `LLTexUnit::getInternalType(...)` usage conversion
- mip level selection
- texture name ownership and deletion order
- texture name 0 detach policy
- FBO bind/restore ordering
- status check timing
- `mUseDepth`, `mTex`, and `mDepth` state updates

## Source Patch Shape

Only this local helper body should change in the first source patch:

- `set_framebuffer_texture_attachment(...)`

Expected delegation:

- local helper calls
  `LLGLContainment::setReadWriteFramebufferTexture2D(...)`
- `clear_framebuffer_texture_attachment(...)` remains a local helper and still
  delegates to `set_framebuffer_texture_attachment(..., 0)`

No public `LLRenderTarget` API should change.

## Behavior That Must Not Move

Do not move these into `llglcontainment.*`:

- `LLTexUnit::getInternalType(...)`
- color attachment index calculation
- texture deletion
- depth sharing policy
- `mUseDepth` updates
- `mTex` or `mDepth` ownership
- framebuffer status checks
- FBO bind/restore helpers

## Expected Behavior Change

Expected behavior change:

- none

This task changes where the raw OpenGL call is issued, not which texture is
attached, detached, deleted, or checked.

## Review Checklist

Reviewers should check:

- `LLGLContainment` gains only one attachment function
- `LLRenderTarget` local helper names still express attach/detach intent
- the local helper still passes `LLTexUnit::getInternalType(usage)`
- the local helper still passes mip level 0
- `clear_framebuffer_texture_attachment(...)` still uses texture name 0
- texture delete order is unchanged
- status check timing is unchanged
- no draw-pool, `pipeline.cpp`, UI, shader, or texture upload code is touched

## Verification Plan

Minimum source verification:

- build `llrender/fast`
- run `git diff --check`
- regenerate generated source inventory

Because this is another behavior-bearing `llglcontainment.*` packet, also run:

- local incremental Xcode arm64 Release build
- executable architecture check
- runtime dylib presence check

Runtime smoke test remains recommended before widening phase 3 beyond
`LLRenderTarget`.
