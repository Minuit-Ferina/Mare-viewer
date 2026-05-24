# Backend shader and texture upload routing summary

## Scope

This batch extends `LLRenderBackend` with transitional operations for:

- shader/program creation, deletion, attachment, linking, validation, binary cache IO, and reflection;
- uniform uploads, uniform blocks, vertex attribute constants, and shader profiling queries;
- legacy post-process state stack calls;
- cubemap array texture allocation and updates;
- `LLImageGL` texture upload, sub-image upload, readback, mipmap generation, texture parameters, texture residency, viewport/binding diagnostics, and sync objects;
- fixed-function material specular state used by `LLGLSSpecular`.

## Files changed

- `indra/llrender/llrenderbackend.h`
- `indra/llrender/llrenderbackend.cpp`
- `indra/llrender/llglslshader.cpp`
- `indra/llrender/llshadermgr.cpp`
- `indra/llrender/llpostprocess.cpp`
- `indra/llrender/llcubemaparray.cpp`
- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llglstates.h`

## Behavior

The OpenGL backend still calls the same `LLGLContainment` helpers. Existing
shader object IDs, program IDs, texture names, sync objects, and program binary
formats remain unchanged.

Some shader reflection values and texture formats are still represented as
numeric backend tokens because surrounding code compares them to existing
OpenGL constants. They are transitional API points, not final Vulkan-facing
interfaces.

## Remaining debt

`indra/llrender/llgl.cpp` still owns OpenGL bootstrap, capability discovery,
diagnostics, and legacy RAII state tracking. It is now the only remaining
non-backend `llrender` file with direct `LLGLContainment` calls.
