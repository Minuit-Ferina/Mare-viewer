# LLRender Call Family Map

Branch: `phase3`

Base branch: `phase2`

## Scope

This document maps active direct OpenGL calls in
`indra/llrender/llrender.cpp` before any source edits to `LLRender` or
`LLTexUnit`.

## Summary

Generated inventory reports 44 active direct OpenGL calls in `llrender.cpp`.
They are not one uniform owner. The file contains several local abstractions:

- `LLTexUnit` texture-unit state and texture binding
- `LLTexUnit` texture parameter policy
- `LLRender` global render initialization
- `LLRender` immediate-render state
- debug/error helpers

## Call Families

### Texture Unit Activation And Binding

Approximate callsites:

- `glActiveTexture(...)`
- `glBindTexture(...)`

Owner: `LLTexUnit`.

Risk: medium-high.

Reason: these calls are tightly coupled to cached texture unit state,
`gGL.mCurrTextureUnitIndex`, `gGL.mDirty`, `mCurrTexType`, `mCurrTexture`,
default white texture binding, cube-map binding, and render-target binding.

Do not route blindly. A source packet must preserve every `gGL.flush()` and
`activate()` ordering point.

### Texture Parameters

Approximate callsites:

- `glTexParameteri(...)`
- `glTexParameterf(...)`

Owner: `LLTexUnit`.

Risk: medium.

Reason: texture address/filtering decisions stay in `LLTexUnit`; containment
can own only raw parameter writes. This is the most plausible first
`llrender.cpp` source packet, but it still needs a focused task because it
touches filtering behavior used everywhere.

### Texture Unit Debug Query

Approximate callsite:

- `glGetIntegerv(GL_ACTIVE_TEXTURE, ...)`

Owner: `LLTexUnit::debugTextureUnit()`.

Risk: low.

Reason: this is debug-only state inspection. It can likely reuse
`LLGLContainment::getInteger(...)`.

### Global Render Initialization

Approximate callsites:

- debug message callback and debug enable on Windows
- pixel-store alignment setup
- cull-face setup
- cube-map seamless enable
- dummy VAO generation and bind
- line-width range queries

Owner: `LLRender::init(...)` and `LLRender::initVertexBuffer()`.

Risk: medium.

Reason: this is startup state. Several calls already have containment helpers,
but VAO and debug callback helpers do not. A safe source packet should start
with pixel-store and range queries only, or define a full init-state contract
first.

### Immediate Render State

Approximate callsites:

- `glColorMask(...)`
- `glBlendFunc(...)`
- `glBlendFuncSeparate(...)`
- `glIsEnabled(GL_LINE_SMOOTH)`
- `glLineWidth(...)`
- `glGetError()`

Owner: `LLRender`.

Risk: medium-high.

Reason: these calls depend on cached blend/color/line state and on explicit
flush ordering. `glLineWidth(...)` can reuse an existing containment helper, but
the line-width clamp depends on `glIsEnabled(...)` and cached max line widths.

## Recommended Source Packet Order

1. `LLRender::initVertexBuffer()` line-width range queries only.
   - Use existing `LLGLContainment::getInteger(...)`.
   - Preserves existing `stop_glerror()` ordering.
2. `LLTexUnit::debugTextureUnit()` active texture query only.
   - Use existing `LLGLContainment::getInteger(...)`.
   - Debug-only, low risk.
3. `LLRender::clearErrors()` error drain only.
   - Use existing `LLGLContainment::getError()`.
   - Keep the loop shape unchanged.
4. Texture parameter writes.
   - Requires a float texture parameter helper before source edits.
   - Keep `LLTexUnit` as policy owner.
5. Binding/activation and blend/color state.
   - Defer until smaller packets above are complete.

## Do Not Touch Yet

- shader owners
- `LLGLState` RAII behavior in `llgl.cpp`
- global texture binding policy without a focused task
- debug callback setup without a Windows-specific review
- `pipeline.cpp`

## Recommendation

The next source packet, if chosen, should be tiny:

- route only the two `LLRender::initVertexBuffer()` line-width range queries
  through `LLGLContainment::getInteger(...)`

That packet needs no new containment API and should not affect runtime order.
