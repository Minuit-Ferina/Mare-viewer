# Phase 3 Render Target Remaining Calls Review

This document reviews the direct OpenGL calls that remain in
`LLRenderTarget` after the completed phase 3 packets.

Branch: `phase3`

Base branch: `phase2`

## Remaining Direct Calls

Current direct OpenGL calls in `indra/llrender/llrendertarget.cpp`:

- `glViewport(...)`: 2
- `glGenerateMipmap(...)`: 1
- `glClear(...)`: 1
- `glScissor(...)`: 1
- `glGetError(...)`: 1

All remaining calls are already grouped behind local phase 2 helpers.

## Remaining Families

### Viewport

Helpers:

- `set_render_target_viewport(...)`
- `restore_default_framebuffer_viewport()`

Risk: high.

Reason:

- the render target viewport is paired with `bindTarget()` / `flush()`
- default framebuffer restore depends on external `gGLViewport`
- mistakes can affect 2D UI alignment, 3D world framing, and resize behavior

Recommendation:

- defer until after lower-risk single-call packets
- require window resize and loaded-scene runtime smoke when changed

### Mipmap Generation

Helper:

- `generate_bound_render_target_mipmaps()`

Risk: medium.

Reason:

- one callsite
- already isolated in a local helper
- ordering remains owned by `LLRenderTarget::flush()`
- local code still controls `mGenerateMipMaps`, texture channel binding, and
  `GL_TEXTURE_2D` target selection

Recommendation:

- choose as the next containment packet

### Clear And Scissor

Helpers:

- `clear_render_target_buffers(...)`
- `set_render_target_scissor(...)`

Risk: medium to high.

Reason:

- clear mutates currently bound framebuffer contents
- fallback scissor path depends on render target dimensions and global scissor
  state
- behavior is visible in world rendering, probes, dynamic textures, and preview
  paths

Recommendation:

- defer until after mipmap
- consider whether clear and scissor should be one packet or two smaller
  packets

### Texture Allocation Error Check

Helper:

- `render_target_texture_allocation_failed()`

Risk: medium.

Reason:

- `glGetError()` consumes OpenGL error state
- moving the check can change which operation is reported as failed
- allocation return behavior depends on the result

Recommendation:

- defer until after mipmap and clear/scissor review
- keep `clear_glerror()` placement and allocation return behavior local

## Next Candidate

The next suitable containment candidate is render target mipmap generation:

- `glGenerateMipmap(...)`

Reason:

- lowest remaining source surface
- already has a phase 2 contract
- already has one local helper
- does not require moving `LLRenderTarget::flush()` ownership
- does not require touching `pipeline.cpp`, draw pools, UI, shader managers,
  or texture upload code

Before source edits, add a task-specific containment note for this mipmap
family.
