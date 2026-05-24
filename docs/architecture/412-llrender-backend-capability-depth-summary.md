# LLRender Backend Capability And Depth Summary

## Scope

This packet routes named capability and depth-state intentions through
`LLRenderBackend`.

Runtime owners touched:

- `indra/llrender/llrender.cpp`
- `indra/llrender/llcubemap.cpp`
- `indra/llrender/llgl.cpp`

Backend files touched:

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`

## What Moved

The OpenGL backend now owns containment calls for:

- debug output synchronous enable;
- texture cube map seamless enable;
- multisample disable during GL state initialization;
- cull face selection;
- depth test enable/disable;
- depth compare function;
- depth write mask.

The abstract interface uses named concepts:

- `LLRenderCapability`
- `LLRenderCullFace`
- `LLRenderDepthFunction`

## What Did Not Move

`LLGLState` still accepts arbitrary `GLenum` state values and therefore remains
an OpenGL-specific state guard. Moving it directly into `LLRenderBackend` would
turn the backend interface into a GL enum tunnel, so it is intentionally left
alone in this packet.

This packet also does not move:

- debug output callback registration;
- generic capability queries;
- GL state validation queries;
- texture state reset;
- synchronization/fence behavior.

## Boundary Notes

The backend header does not expose OpenGL constants. The OpenGL backend maps
named backend enums to OpenGL constants internally and continues to call
`LLGLContainment`.

## Verification

- Built `llrender/CMakeFiles/llrender.dir/llrenderbackend.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llrender.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llcubemap.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llgl.cpp.o`.
- Re-archived `llrender/libllrender.a`.
