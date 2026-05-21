# LLRenderTarget Local Intent Map

This document summarizes the local OpenGL intent naming completed in
`LLRenderTarget` during phase 2.

Related files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

## Current State

All direct OpenGL calls in `indra/llrender/llrendertarget.cpp` are now grouped
behind internal helpers in the `.cpp` file.

This is not a new rendering abstraction and it is not an `LLGLContainment`
API. The owner remains `LLRenderTarget`.

## Local Helper Families

### Framebuffer Status

Helper:

- `check_current_draw_framebuffer_status()`

Intent:

- debug-check the currently bound draw framebuffer

Contract document:

- `docs/architecture/22-llrendertarget-framebuffer-status-contract.md`

### Viewport

Helpers:

- `set_render_target_viewport(...)`
- `restore_default_framebuffer_viewport()`

Intent:

- apply a render target-local viewport
- restore the default framebuffer viewport from `gGLViewport`

Contract document:

- `docs/architecture/15-llrendertarget-viewport-contract.md`

### FBO Binding

Helpers:

- `bind_render_target_fbo(...)`
- `bind_attachment_fbo(...)`
- `restore_tracked_fbo_binding()`
- `bind_default_framebuffer_for_flush()`
- `forget_current_fbo_and_bind_default()`

Intent:

- separate draw-time FBO binding from temporary attachment mutation binding
- preserve existing `sCurFBO` update points

Contract document:

- `docs/architecture/16-llrendertarget-fbo-contract.md`

### FBO Lifetime

Helpers:

- `generate_framebuffer_name(...)`
- `delete_framebuffer_name(...)`

Intent:

- name FBO object lifetime operations while keeping ownership local to `mFBO`

Contract document:

- `docs/architecture/19-llrendertarget-fbo-lifetime-contract.md`

### Texture Attachments

Helpers:

- `set_framebuffer_texture_attachment(...)`
- `clear_framebuffer_texture_attachment(...)`

Intent:

- name FBO texture attach and detach operations

Contract document:

- `docs/architecture/18-llrendertarget-attachment-contract.md`

### Buffer Routing

Helpers:

- `set_render_target_buffer_routing(...)`
- `restore_default_framebuffer_buffer_routing()`

Intent:

- configure draw/read buffers for render target color attachments
- restore default framebuffer back-buffer routing

Contract document:

- `docs/architecture/17-llrendertarget-buffer-routing-contract.md`

### Mipmap Generation

Helper:

- `generate_bound_render_target_mipmaps()`

Intent:

- generate mipmaps for the currently bound render target color texture during
  `flush()`

Contract document:

- `docs/architecture/20-llrendertarget-mipmap-contract.md`

### Clear And Scissor

Helpers:

- `clear_render_target_buffers(...)`
- `set_render_target_scissor(...)`

Intent:

- clear the selected render target buffers
- constrain fallback clear behavior to the target rectangle

Contract document:

- `docs/architecture/21-llrendertarget-clear-contract.md`

### Texture Allocation Error Checks

Helper:

- `render_target_texture_allocation_failed()`

Intent:

- check whether the current render target texture allocation reported a GL
  error

Contract document:

- `docs/architecture/23-llrendertarget-texture-allocation-error-contract.md`

## Boundary Decision

Keep these helpers local for now.

Reasons:

- the helpers depend on `LLRenderTarget` state such as `mFBO`, `mTex`,
  `mDepth`, `mUsage`, `sCurFBO`, `sCurResX`, `sCurResY`, and `sBoundTarget`
- the helpers encode ordering assumptions inside `bindTarget()`, `flush()`,
  allocation, attachment mutation, and release
- moving them to `llglcontainment.*` now would create a generic OpenGL wrapper
  before a reusable containment contract exists

## Remaining Risk

Risk level: medium.

This cleanup makes intent easier to review, but it does not change runtime
behavior or reduce the number of OpenGL operations. The main remaining risks
are the existing global-state dependencies:

- render target stack ordering
- `sCurFBO` consistency
- `gGLViewport` freshness
- texture allocation ownership and error-state consumption
- nested render target restore behavior

## Next Suitable Phase 2 Work

Small next steps should avoid widening this patch into a renderer abstraction.
Good candidates:

- document `LLImageGL` texture upload/lifetime call families
- document `LLVertexBuffer` buffer binding/update call families
- document `LLGLSLShader` shader program call families
- run one incremental Xcode integration build before declaring this
  `LLRenderTarget` packet ready for review

Do not move these helpers to `llglcontainment.*` until a phase 3 task defines a
stable cross-owner containment contract.
