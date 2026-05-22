# Draw Pool Alpha Shader Selection Map

Date: 2026-05-23

## Scope

This document maps the normal alpha shader-selection logic in
`LLDrawPoolAlpha::renderAlpha(...)`.

It is docs-only. It does not propose a source patch.

## Owner Files

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Relevant Existing Preparation

`renderPostDeferred(...)` prepares these member shader pointers before
`renderAlpha(...)` runs:

- `emissive_shader`
- `pbr_emissive_shader`
- `fullbright_shader`
- `simple_shader`
- material shader array: `gDeferredMaterialProgram`
- `pbr_shader`

The prepared shader choice depends on:

- impostor rendering;
- HUD rendering;
- deferred alpha mode;
- material shader mask;
- PBR/GLTF alpha mode;
- rigged versus non-rigged draw info.

`LLGLSLShader::unbind()` is called after shader preparation so the alpha render
loop does not inherit the last prepared shader by accident.

## Per-Draw Selection Flow

For each `LLDrawInfo` in the selected alpha draw map:

1. Reject draw info that does not match the current rigged/non-rigged pass.
2. Apply the model matrix before shader/material selection.
3. Load:
   - `mat = nullptr`;
   - `gltf_mat = params.mGLTFMaterial`.
4. Disable face culling when the GLTF material is double-sided.

## GLTF Blend Path

Condition:

```cpp
gltf_mat && gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND
```

Behavior:

- choose `pbr_shader`;
- switch to `pbr_shader->mRiggedVariant` when `params.mAvatar != nullptr`;
- bind the target shader with `gPipeline.bindDeferredShaderFast(...)` if it is
  not already current;
- bind the GLTF material with `params.mGLTFMaterial->bind(params.mTexture)`.

Important details:

- the non-GLTF material/fullbright/simple branch is skipped;
- `mat` remains `nullptr`;
- the later `TexSetup(&params, mat != nullptr)` call passes `false`;
- `TexSetup(...)` still handles GLTF texture-matrix setup through
  `draw->mGLTFMaterial`.

## Non-GLTF Alpha Path

Condition:

```cpp
!(gltf_mat && gltf_mat->mAlphaMode == LLGLTFMaterial::ALPHA_MODE_BLEND)
```

Behavior:

1. Set `mat`:
   - HUD rendering forces `mat = nullptr`;
   - otherwise `mat = params.mMaterial`.
2. Update `light_enabled` and `initialized_lighting`:
   - fullbright draw info marks lighting disabled and seeds
     `target_shader = fullbright_shader`;
   - non-fullbright draw info marks lighting enabled and seeds
     `target_shader = simple_shader`.
3. Choose final base shader:
   - HUD rendering: `fullbright_shader`;
   - material draw: `gDeferredMaterialProgram[params.mShaderMask]`;
   - non-fullbright draw: `simple_shader`;
   - otherwise: `fullbright_shader`.
4. If rigged, switch the selected shader to its rigged variant.
5. Bind the selected shader when it is not already current.
6. When binding a fullbright draw, enable and bind `LLShaderMgr::EXPOSURE_MAP`
   if the shader exposes that texture channel.
7. Set non-GLTF material uniforms:
   - default specular color: `(1, 1, 1, 1)`;
   - default environment intensity: `0`;
   - default emissive brightness: `1`;
   - material draw overrides specular color and environment intensity from
     `params`;
   - material draw sets brightness to `1` only for fullbright material draws,
     otherwise `0`.

## Rigged Handling

Rigged handling happens after base shader selection:

- assert the selected shader has `mRiggedVariant`;
- replace `target_shader` with `target_shader->mRiggedVariant`;
- upload the matrix palette after shader selection and before texture setup;
- skip the draw when matrix-palette upload fails.

This ordering must stay intact. The matrix palette depends on the chosen shader
variant and cached avatar/mesh state.

## Texture Setup Coupling

The shader-selection branch controls the later `TexSetup(...)` call through
the `mat != nullptr` argument.

Consequences:

- GLTF blend path passes `false`, but GLTF-specific texture-matrix setup still
  runs because `draw->mGLTFMaterial` is present.
- Non-GLTF material path passes `true`, allowing normal/specular/material
  texture binding through the current shader.
- Simple non-material path can bind fallback flat normal and white specular
  textures when the current shader is `simple_shader` or its rigged variant.
- Texture matrix setup increments `gPipeline.mTextureMatrixOps` and must be
  restored by `RestoreTexSetup(...)`.

## Blend And Minimum Alpha Coupling

After shader and texture setup:

- blend factors are set from `params.mBlendFuncSrc` and
  `params.mBlendFuncDst`;
- if not impostor rendering and neither blend factor is `BF_SOURCE_ALPHA`,
  minimum alpha is temporarily set to `0`;
- the draw call runs;
- minimum alpha is restored to `MINIMUM_ALPHA` when it was changed.

This is per-draw state and should not be separated from the draw call without a
dedicated behavior task.

## Emissive Queue Coupling

After the main alpha draw:

- emissive/glow queueing is skipped for `POOL_ALPHA_PRE_WATER`;
- queue selection depends on:
  - rigged versus non-rigged;
  - GLTF material present versus absent;
  - emissive vertex-buffer data.

The queued emissive pass later restores the normal alpha blend mode and may
rebind the previous shader.

## Invariants

Do not change these without a dedicated behavior task:

- GLTF blend shader must be bound before `LLGLTFMaterial::bind(...)`;
- HUD rendering must force the fullbright alpha path;
- material alpha must use `gDeferredMaterialProgram[params.mShaderMask]`;
- `params.mShaderMask` must remain asserted against
  `LLMaterial::SHADER_COUNT`;
- rigged variant selection must happen after the base shader is selected;
- exposure map binding must remain tied to fullbright shader binding;
- non-GLTF material uniforms must remain outside the GLTF blend path;
- matrix-palette upload must remain after shader selection and before draw;
- `TexSetup(...)` must receive `mat != nullptr`;
- texture-matrix restore must happen after each draw that set texture state;
- per-draw blend override and minimum-alpha restore must remain near the draw;
- final `gGL.setSceneBlendType(LLRender::BT_ALPHA)` must remain after the
  alpha groups finish.

## Risk

Risk level: high.

Reasons:

- shader selection mixes render mode, material mode, rigged mode, HUD mode,
  lighting state, and exposure-map binding;
- small ordering changes can produce visible alpha regressions;
- GLTF and legacy material paths deliberately diverge;
- `mat` is both a material pointer and an input to later texture setup policy;
- the currently disabled RLV/PBR block is located inside this branch and should
  not be moved accidentally.

## Source Patch Guidance

No source patch should be made from this map alone.

Before a source patch, add a task-specific note that names exactly one small
operation, for example:

- extracting only the GLTF blend shader target decision;
- extracting only the non-GLTF base shader choice;
- extracting only the exposure-map binding condition;
- extracting only the material-uniform parameter calculation.

Each of those should be reviewed separately.

## Verification Plan For Future Source Patch

Required command checks:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Recommended runtime scenes before accepting behavior changes:

- transparent non-material object;
- transparent material object;
- GLTF alpha-blend object;
- fullbright alpha object;
- rigged alpha attachment;
- HUD alpha attachment;
- impostor alpha view if practical;
- transparent objects around water.
