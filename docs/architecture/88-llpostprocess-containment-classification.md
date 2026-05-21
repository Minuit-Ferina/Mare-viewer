# LLPostProcess Containment Classification

Branch: `phase3`

Base branch: `phase2`

## Scope

This document classifies active direct OpenGL calls in
`indra/llrender/llpostprocess.cpp` before source changes.

## Call Families

### State Stack And Clear

Callsites:

- `glPushAttrib(GL_ALL_ATTRIB_BITS)`
- `glPushClientAttrib(GL_ALL_ATTRIB_BITS)`
- `glClearColor(...)`
- `glClear(GL_COLOR_BUFFER_BIT)`
- `glPopClientAttrib()`
- `glPopAttrib()`

Risk: medium.

Reason: these calls bracket postprocess rendering and depend on exact ordering.
The wrapper must not change the pushed bit mask, clear color, clear mask, or
pop order.

Priority: first source packet.

### Texture Copy And Allocation

Callsites:

- `glCopyTexImage2D(GL_TEXTURE_RECTANGLE, ...)`
- `glTexImage2D(GL_TEXTURE_RECTANGLE, ...)`

Risk: low-medium.

Reason: `LLPostProcess` still owns texture choice, dimensions, format, and
binding. Containment can own only the raw copy/allocation calls.

Priority: second source packet.

### Shader And Error Queries

Callsites:

- `glGetUniformLocation(...)`
- `glGetError()`
- `glGetShaderiv(...)`
- `glGetProgramInfoLog(...)`

Risk: low-medium.

Reason: these calls query OpenGL state and logs. The wrapper must preserve the
existing loop behavior in `checkError()` and the existing log retrieval in
`checkShaderError(...)`.

Priority: third source packet.

## Out Of Scope

Do not touch in this containment pass:

- effect selection
- shader creation policy
- shader uniform ownership
- matrix setup
- texture binding
- postprocess render order
- night vision, bloom, or color filter behavior

## Verification Plan

- Run `git diff --check`.
- Run one targeted `llrender/fast` build after the wrapper-only source packets.
- Regenerate source inventory.

No Xcode or runtime smoke is required for these pure wrapper packets.
