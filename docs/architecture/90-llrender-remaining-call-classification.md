# LLRender Remaining Direct Call Classification

Branch: `phase3`

Base branch: `phase2`

## Scope

This document classifies the remaining `indra/llrender` direct-call entries
after the `LLRenderTarget`, `LLVertexBuffer`, `LLImageGL`, cubemap,
`LLRender2DUtils`, `LLGLStates`, and `LLPostProcess` containment packets.

## False Positives Or Non-Actionable Entries

These entries are reported by the generated inventory but do not require source
containment right now:

- `indra/llrender/llimagegl.cpp`
  - remaining `glTexParameteri(...)` matches are inside a disabled block comment
  - the other raw match is explanatory text
- `indra/llrender/llimagegl.h`
  - matches are comments documenting texture helper ownership
- `indra/llrender/llvertexbuffer.cpp`
  - remaining sync matches are inside disabled/commented code
- `indra/llrender/llglcommonfunc.cpp`
  - remaining stencil matches are commented deprecated code
- `indra/llrender/llgl.h`
  - remaining match is explanatory state-management documentation
- `indra/llrender/llcubemaparray.cpp`
  - remaining mipmap match is a disabled AMD-driver workaround comment
- `indra/llrender/llrender2dutils.cpp`
  - remaining raw reference is non-call text
- `indra/llrender/llcubemap.cpp`
  - remaining raw reference is non-call text
- `indra/llrender/llpostprocess.cpp`
  - active direct calls are now 0; remaining raw references are non-call text

## Remaining Active Owners

### `indra/llrender/llglslshader.cpp`

Active direct calls: 80.

Risk: high.

Reason: owns shader program lifetime, query objects, uniform discovery,
uniform updates, attach/detach, program bind/unbind, and shader diagnostics.
This is not a small mechanical wrapper packet.

Recommended next step: write a shader-owner contract before edits.

### `indra/llrender/llgl.cpp`

Active direct calls: 79.

Risk: high.

Reason: this file is already the core OpenGL state/platform owner. Routing it
blindly through containment may just add indirection inside the current low-level
owner without improving ownership.

Recommended next step: classify which calls are platform discovery, state RAII,
debug, depth state, and sync before selecting any source packet.

### `indra/llrender/llrender.cpp`

Active direct calls: 44.

Risk: medium-high.

Reason: owns `LLTexUnit`, `LLRender`, texture binding, texture parameters,
global render initialization, blend/color/line state, and error draining.
Small packets are possible, but they need local contracts because this file is
already a central render abstraction.

Recommended next step: create a focused `LLRender`/`LLTexUnit` call-family map.

### `indra/llrender/llshadermgr.cpp`

Active direct calls: 32.

Risk: high.

Reason: owns shader compile/link, binary cache, error handling, and program
validation. It overlaps with helpers introduced for `LLPostProcess`, but it
should not be routed without a shader lifecycle contract.

Recommended next step: group shader compile/link/binary-cache calls before edits.

### `indra/llrender/llglheaders.h`

Reported direct calls: declarations only.

Risk: do not edit for containment.

Reason: this header declares OpenGL extension entry points. It is not a runtime
callsite owner.

Recommended next step: leave unchanged.

## Recommendation

Stop pure small-wrapper source packets here.

The next useful step is documentation, not code: map `llrender.cpp` call
families and decide which, if any, should move to containment. That keeps the
branch reviewable and avoids turning `llglcontainment.*` into an unstructured
duplicate of the existing low-level renderer.
