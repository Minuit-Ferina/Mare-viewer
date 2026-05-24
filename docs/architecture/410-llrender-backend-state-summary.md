# LLRender Backend State Summary

## Scope

This packet routes a batch of simple render-state intentions through
`LLRenderBackend`.

Runtime owners touched:

- `indra/llrender/llrender.cpp`
- `indra/llrender/llrender2dutils.cpp`
- `indra/llrender/llpostprocess.cpp`

Backend files touched:

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`

## What Moved

The OpenGL backend now owns the direct containment calls for:

- clear color;
- color write mask;
- blend state;
- line width.

The abstract interface uses backend-neutral concepts:

- `LLRenderClearColor`
- `LLRenderColorMask`
- `LLRenderBlendState`
- `LLRenderBlendFactor`

## What Did Not Move

This packet does not move:

- texture binding or texture parameters;
- shader and uniform handling;
- vertex/index buffers;
- draw calls;
- framebuffer binding;
- matrix state;
- GL state push/pop;
- line-width range queries.

Those families still need separate vocabulary before they are good backend
abstraction candidates.

## Boundary Notes

`llrenderbackend.h` still does not expose OpenGL constants or OpenGL ABI types.

The OpenGL backend implementation maps `LLRenderBlendFactor` to OpenGL blend
constants internally and still calls `LLGLContainment`, not raw `gl*`.

## Verification

- Built `llrender/CMakeFiles/llrender.dir/llrenderbackend.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llrender.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llrender2dutils.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llpostprocess.cpp.o`.
- Re-archived `llrender/libllrender.a`.
