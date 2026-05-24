# Render Target Backend Viewport/Scissor Summary

## Scope

This packet adds the first tiny runtime use of the backend interface.

Runtime owner touched:

- `indra/llrender/llrendertarget.cpp`

Backend files touched:

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`

## What Moved

`LLRenderTarget` now routes these local intentions through
`getOpenGLRenderBackend()`:

- render target viewport application;
- default framebuffer viewport restoration;
- render target scissor box setup.

The OpenGL backend still calls `LLGLContainment`. It does not call `gl*`
directly.

## What Did Not Move

This packet does not move:

- framebuffer binding;
- framebuffer status checks;
- draw/read buffer routing;
- clear masks;
- clear color/depth/stencil values;
- texture attachments;
- mipmap generation;
- draw calls;
- shader or buffer ownership.

`beginRenderPass()` and `endRenderPass()` remain compile-checked vocabulary only
for now.

## Threading Notes

The OpenGL backend implementation still assumes the current legacy OpenGL
context model. The abstract interface must not treat that as the future backend
contract.

Before Vulkan or multi-threaded recording is introduced, the backend contract
must define:

- command recording ownership;
- command submission ownership;
- which operations are safe from worker threads;
- which operations must stay on the main/render thread.

## Verification

- Built `llrender/CMakeFiles/llrender.dir/llrenderbackend.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llrendertarget.cpp.o`.
- Re-archived `llrender/libllrender.a`.
