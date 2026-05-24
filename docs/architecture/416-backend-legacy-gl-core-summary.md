# Backend legacy OpenGL core routing summary

## Scope

This batch routes the remaining `llgl.cpp` OpenGL bootstrap and legacy state
calls through `LLRenderBackend`.

The new backend methods are deliberately named `legacy*` where they still pass
OpenGL-era numeric tokens. They are containment adapters for the current GL
manager, not the final Vulkan command vocabulary.

## Files changed

- `indra/llrender/llgl.cpp`
- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`
- `indra/llrender/llrender.cpp`
- `indra/llrender/llcubemap.cpp`

## Behavior

The OpenGL backend still calls the same `LLGLContainment` helpers. Existing
capability discovery, extension string parsing, GL diagnostics, RAII state
tracking, depth-state verification, client active texture reset, and sync fence
behavior remain unchanged.

## Result

Outside of `llrenderbackend.cpp` and `llglcontainment.cpp`, `indra/llrender`
no longer calls `LLGLContainment` directly.
