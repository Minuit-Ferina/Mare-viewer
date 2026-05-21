# LLRender Windows Debug Callback Decision

Branch: `phase3`

Base branch: `phase2`

## Scope

This document records the decision for the remaining active direct OpenGL calls
in `indra/llrender/llrender.cpp` after the `LLTexUnit` binding containment
packet.

Remaining active direct calls:

- `glDebugMessageCallback(...)`
- `glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS)`

The generated inventory also reports the commented
`glDebugMessageControl(...)` line as a raw reference.

## Decision

Leave these calls direct for now.

## Reason

This block is Windows-only debug callback wiring under `#if LL_WINDOWS`.
Wrapping it through `llglcontainment.h` would require exposing callback-related
OpenGL types such as `GLDEBUGPROC` through the containment interface, or adding
platform-specific declarations to a header that is currently intentionally
narrow and based on `llgltypes.h`.

That would add interface complexity for a debug initialization path that is not
part of the normal Darwin render path being exercised in this batch.

## Risk

Low if left direct:

- the calls are platform-specific debug setup
- they are not part of regular frame rendering
- the current local build target is Darwin arm64, where this block is excluded

Medium if wrapped now:

- the containment header would need broader OpenGL callback type exposure
- Windows-specific behavior would be changed without a Windows build check
- the wrapper would not be validated by the current local Darwin build path

## Next Step

Do not change this block in the current wrapper batch.

If Windows debug callback containment becomes desirable, create a separate task
with a Windows build or review path and define a callback-specific containment
API intentionally.
