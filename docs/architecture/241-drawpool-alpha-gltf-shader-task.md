# Draw Pool Alpha GLTF Shader Task

Date: 2026-05-23

## Scope

This task allows one narrow source cleanup in `LLDrawPoolAlpha`.

It may extract only the GLTF alpha-blend shader target decision from
`LLDrawPoolAlpha::renderAlpha(...)`.

## Owner File

- `indra/newview/lldrawpoolalpha.cpp`

## Allowed Source Change

Allowed:

- add an implementation-local helper that receives the prepared `pbr_shader`
  pointer and the current `LLDrawInfo`;
- return `pbr_shader` for non-rigged GLTF alpha-blend draws;
- return `pbr_shader->mRiggedVariant` when `params.mAvatar != nullptr`;
- replace only the existing inline GLTF alpha target assignment with that
  helper call.

## Behavior To Preserve

Preserve exactly:

- GLTF alpha-blend condition;
- base shader pointer: `pbr_shader`;
- rigged test: `params.mAvatar != nullptr`;
- no new assert on `mRiggedVariant`;
- shader bind condition: `current_shader != target_shader`;
- shader bind method: `gPipeline.bindDeferredShaderFast(*target_shader)`;
- material bind order: shader bind before `params.mGLTFMaterial->bind(...)`;
- `mat` remains `nullptr`;
- later `TexSetup(&params, mat != nullptr)` still receives `false`.

## Not Allowed

Do not change:

- non-GLTF material/fullbright/simple shader selection;
- exposure-map binding;
- material uniforms;
- texture setup;
- blend factors;
- minimum-alpha logic;
- emissive queueing;
- RLV/PBR commented block placement.

## Risk

Risk level: low-medium.

The helper is simple, but it lives in a high-risk function. The key risk is
accidentally changing GLTF material bind ordering or coupling to later texture
setup.

## Verification

Required:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Runtime smoke:

- can be deferred if the patch only moves the shader pointer choice into a
  helper and preserves the exact bind/material order.
