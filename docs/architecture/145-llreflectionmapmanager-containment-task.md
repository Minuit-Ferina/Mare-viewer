# LLReflectionMapManager Containment Task

Branch: `phase3`
Source owner: `LLReflectionMapManager`

## Scope

Route direct OpenGL calls in `indra/newview/llreflectionmapmanager.cpp`
through `llglcontainment.*`.

Included raw OpenGL families:
- `glTexImage2D`
- `glGenerateMipmap`
- `glCopyTexSubImage3D`
- `glViewport`
- `glGenBuffers`
- `glBindBuffer`
- `glBufferData`
- `glBindBufferBase`
- `glDeleteBuffers`

Included behavior:
- EXR preview texture allocation and mipmap generation
- reflection probe cube-array mip copy
- radiance and irradiance viewport sizing
- reflection probe UBO lifetime and binding

## Non-Scope

Do not change:
- reflection probe scheduling
- cube face order
- mip chain sizing
- radiance versus irradiance pass selection
- UBO data layout
- texture ownership
- render target binding order

Do not move reflection-map rendering into another owner.

## Ownership Notes

`LLReflectionMapManager` keeps ownership of:
- reflection probe update flow
- cube-array layer selection
- mip level selection
- viewport dimensions
- UBO allocation timing and data contents

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers
- API names for texture copy, texture storage, viewport, and UBO operations

## Risk

Risk: medium.

Why:
- This is wrapper-only, but it touches active reflection probe texture and UBO
  paths.
- Exact mip, face, viewport, and buffer binding order must stay unchanged.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llreflectionmapmanager.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later in a scene with reflection probes
  enabled.
