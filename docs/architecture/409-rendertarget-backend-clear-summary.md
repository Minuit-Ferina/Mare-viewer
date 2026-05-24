# Render Target Backend Clear Summary

## Scope

This packet routes the `LLRenderTarget::clear()` buffer-clear intent through the
backend interface.

Runtime owner touched:

- `indra/llrender/llrendertarget.cpp`

Backend and containment files touched:

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`

## What Moved

`LLRenderTarget` still owns the legacy GL-mask input from
`LLRenderTarget::clear(U32 mask_in)`. It converts the final color/depth/stencil
intent into the backend-neutral `LL_RENDER_CLEAR_*` mask and calls
`getOpenGLRenderBackend().clear(desc)`.

The OpenGL backend delegates the final GL mask translation to
`LLGLContainment::clearBuffersByIntent()`.

## What Did Not Move

This packet does not move:

- clear color ownership;
- clear depth ownership;
- clear stencil ownership;
- framebuffer binding;
- framebuffer status checks;
- scissor enable/disable state;
- render pass begin/end;
- draw calls.

## Boundary Notes

The backend interface does not expose `GL_COLOR_BUFFER_BIT`,
`GL_DEPTH_BUFFER_BIT`, or `GL_STENCIL_BUFFER_BIT`.

Those constants remain on the legacy owner side while `LLRenderTarget::clear()`
still accepts a GL-style mask, and on the OpenGL containment side where backend
intent is translated into the actual OpenGL call.

## Verification

- Built `llrender/CMakeFiles/llrender.dir/llglcontainment.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llrenderbackend.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llrendertarget.cpp.o`.
- Re-archived `llrender/libllrender.a`.
