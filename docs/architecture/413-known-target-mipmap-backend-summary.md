# Known Target Mipmap Backend Summary

## Scope

This packet routes mipmap generation for known texture targets through
`LLRenderBackend`.

Runtime owners touched:

- `indra/llrender/llcubemap.cpp`
- `indra/llrender/llrendertarget.cpp`

Backend files touched:

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`

## What Moved

The backend interface now includes `LLRenderTextureTarget` and
`generateMipmaps()`.

Moved callsites:

- cubemap mipmap generation uses `LLRenderTextureTarget::TextureCubeMap`;
- render target mipmap generation uses `LLRenderTextureTarget::Texture2D`.

## What Did Not Move

Generic `LLImageGL` mipmap generation still uses the existing containment path
because it currently carries a GL texture target in `mTarget`. That needs a
separate texture target mapping before it should move to the backend.

This packet also does not move texture allocation, texture upload, texture
binding, or sampler state.

## Boundary Notes

The backend header does not expose `GL_TEXTURE_2D` or `GL_TEXTURE_CUBE_MAP`.
The OpenGL backend maps `LLRenderTextureTarget` internally and calls
`LLGLContainment`.

## Verification

- Built `llrender/CMakeFiles/llrender.dir/llrenderbackend.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llrendertarget.cpp.o`.
- Built `llrender/CMakeFiles/llrender.dir/llcubemap.cpp.o`.
- Re-archived `llrender/libllrender.a`.
