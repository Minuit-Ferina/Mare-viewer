# Backend texture, buffer, and framebuffer routing summary

## Scope

This batch extends `LLRenderBackend` with transitional vocabulary for:

- texture units, texture binding, texture filters, address modes, texture names, and known mipmap targets;
- pixel-store state used by texture upload paths;
- buffer object names, binding, storage, sub-data upload, vertex attributes, and draw calls;
- framebuffer names, framebuffer binding, texture attachments, draw/read buffer routing, and framebuffer completeness checks;
- debug callback setup, dummy vertex array setup, and render backend error polling.

## Files changed

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`
- `indra/llrender/llrender.cpp`
- `indra/llrender/llrender2dutils.cpp`
- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llvertexbuffer.cpp`

## Behavior

The OpenGL backend still maps each operation to the same `LLGLContainment`
helper family. The call order and existing object lifetime remain unchanged.

The backend interface remains free of `GL_*` constants. Existing raw OpenGL
object names are still used as transitional resource identifiers because the
current renderer owns OpenGL-created texture, buffer, vertex array, and
framebuffer names.

## Remaining debt

- `LLImageGL` still contains texture upload/readback calls that need a larger
  texture data vocabulary before they can move cleanly.
- Shader creation, program linking, uniform upload, and shader introspection
  remain mostly in `LLGLSLShader` and `LLShaderMgr`.
- `LLGLState` and fixed-function material calls remain in the legacy OpenGL
  state layer.
