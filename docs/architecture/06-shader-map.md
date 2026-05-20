# Shader Map

This document maps the current shader manager layers and shader asset families.
It is a phase 1 analysis artifact only. It does not propose source edits.

## Files Inspected

- `indra/llrender/llglslshader.h`
- `indra/llrender/llglslshader.cpp`
- `indra/llrender/llshadermgr.h`
- `indra/llrender/llshadermgr.cpp`
- `indra/newview/llviewershadermgr.h`
- `indra/newview/llviewershadermgr.cpp`
- `indra/newview/app_settings/shaders/`
- `indra/newview/CMakeLists.txt`

## Layer Model

| layer | primary files | role | risk |
|---|---|---|---|
| Program wrapper | `llglslshader.*` | Owns `LLGLSLShader` program objects, shader file lists, defines, uniforms, texture channels, GLTF variants, profiling queries, and bind/unbind behavior. | Critical |
| Compile/link manager | `llshadermgr.*` | Loads shader files, chooses shader class fallback, injects GLSL version/defines, compiles, links, validates, manages reserved uniforms/attributes, and handles shader binary cache. | Critical |
| Viewer shader registry | `llviewershadermgr.*` | Declares global viewer shader handles, sets feature flags, assigns shader files, loads shader groups, creates rigged/GLTF variants, and propagates uniforms. | Critical |
| Shader assets | `app_settings/shaders/` | GLSL source grouped by class and family. Higher classes override lower classes when available. | High |

## Loading Flow

`LLViewerShaderMgr::setShaders()` is the high-level entry point:

1. Returns early when the pipeline or shader manager is not initialized.
2. Initializes the shader binary cache.
3. Clears compiled vertex/fragment object maps.
4. Calls `initAttribsAndUniforms()`.
5. Releases GL buffers and unloads existing shaders.
6. Assigns shader levels for lighting, interface, environment, water, object,
   effect, windlight, and deferred groups.
7. Calls `loadBasicShaders()` first.
8. Loads water, effects, interface, object, avatar, and deferred shader groups.
9. Persists shader cache metadata when not a second viewer instance.
10. Recreates GL buffers and finalizes the shader list.

`LLViewerShaderMgr::loadBasicShaders()` loads dependency shader objects used by
later programs. It also builds `LLGLSLShader::sGlobalDefines` from runtime
settings such as shadows, screen-space reflections, reflection probes, mirrors,
terrain PBR settings, skinning limits, and emissive buffer settings.

`LLShaderMgr::loadShaderFile()` searches shader classes from the requested class
down to class 1:

```text
app_settings/shaders/class<requested>/<family>/<file>
app_settings/shaders/class<requested - 1>/<family>/<file>
...
app_settings/shaders/class1/<family>/<file>
```

The selected file is read, GLSL version and generated defines are injected, and
then OpenGL compile/link objects are created through direct `gl*` calls.

`LLGLSLShader::createShader()` owns one viewer program creation attempt:

1. Clears previous program state.
2. Creates a GL program.
3. Attempts cached binary load.
4. Loads and attaches every file in `mShaderFiles` if no binary is available.
5. Attaches feature-driven dependency shaders through `LLShaderMgr`.
6. Maps attributes and uniforms.
7. Falls back to a lower shader level on link/map failure.
8. Sets indexed texture channel uniforms when needed.

## Feature Flags

`LLShaderFeatures` is the main dependency switchboard. Important flags include:

| feature | likely effect |
|---|---|
| `calculatesLighting`, `hasLighting`, `isAlphaLighting`, `isSpecular` | Pulls lighting helper shader objects into the program. |
| `calculatesAtmospherics`, `hasAtmospherics`, `hasGamma`, `hasSrgb` | Pulls windlight/atmospheric/gamma/sRGB helpers into the program. |
| `hasSkinning`, `hasObjectSkinning` | Uses avatar or object skinning helpers and rigged variants. |
| `mGLTF` | Marks GLTF programs and enables GLTF variant handling. |
| `isDeferred`, `hasFullGBuffer`, `hasShadows`, `hasAmbientOcclusion` | Pulls deferred utility, G-buffer, shadow, and AO-related dependencies. |
| `hasScreenSpaceReflections`, `hasReflectionProbes`, `hasHeroProbes` | Pulls SSR/reflection-probe code paths and defines. |
| `hasAlphaMask`, `isPBRTerrain`, `hasTonemap` | Selects material/terrain/post-processing behavior. |
| `mIndexedTextureChannels` | Enables generated indexed texture lookup code and shifts reserved texture uniforms. |
| `attachNothing` | Skips feature attachment entirely. |

Risk: these flags are high blast radius because a change can alter which helper
objects are attached to many shader programs without changing their explicit
`mShaderFiles` lists.

## Shader Asset Families

There are 240 `.glsl` files under `indra/newview/app_settings/shaders/`.

| family | files | likely role | priority |
|---|---:|---|---|
| `class1/deferred` | 117 | Main deferred renderer, G-buffer, PBR, shadows, post, anti-aliasing, Mare velocity/TAA/NIS, FSR2 compute files. | P0 |
| `class1/interface` | 44 | UI, copy, highlight, debug, occlusion, reflection filtering, terrain bake, probe display helpers. | P1 |
| `class3/deferred` | 15 | Higher-end deferred overrides: lighting, haze, SSR, materials, shiny/fullbright. | P1 |
| `class1/objects` | 14 | Forward/object preview, bump, impostor, simple object helpers. | P1 |
| `class1/lighting` | 9 | Basic lighting helpers attached by feature flags. | P0 |
| `class1/windlight` | 8 | Atmospherics, gamma, windlight helpers attached by feature flags. | P0 |
| `class2/deferred` | 8 | Mid/high deferred overrides for alpha, sun, SSAO, soften, reflection probes. | P1 |
| `class1/avatar` | 6 | Avatar, eyeball, avatar/object skinning helpers. | P1 |
| `class1/environment` | 4 | Water, underwater, water fog, sRGB helper. | P1 |
| `class1/effects` | 4 | Glow and glow extraction. | P2 |
| `class2/interface` | 3 | Higher-class reflection/irradiance interface helpers. | P2 |
| `class1/gltf` | 2 | GLTF metallic-roughness program files. | P1 |
| `class3/environment` | 2 | Higher-class water/underwater overrides. | P2 |
| `class3/lighting` | 2 | Higher-class lighting overrides. | P2 |
| root error shaders | 2 | Disabled fallback path for error vertex/fragment shaders. | P3 |

## Viewer Shader Groups

`LLViewerShaderMgr::EShaderClass` defines these groups:

- `SHADER_LIGHTING`
- `SHADER_OBJECT`
- `SHADER_AVATAR`
- `SHADER_ENVIRONMENT`
- `SHADER_INTERFACE`
- `SHADER_EFFECT`
- `SHADER_WINDLIGHT`
- `SHADER_WATER`
- `SHADER_DEFERRED`

Current `setShaders()` assigns fixed class levels in code:

| group | current class level |
|---|---:|
| lighting | 3 |
| interface | 2 |
| environment | 2 |
| object | 2 |
| effect | 2 |
| windlight | 2 |
| water | 3 |
| deferred | 3 |

These are then passed to the class fallback search in `LLShaderMgr`.

## Major Program Families

| loader | program families |
|---|---|
| `loadBasicShaders()` | windlight helpers, lighting helpers, skinning helpers, texture lookup helpers, deferred utility fragments. |
| `loadShadersWater()` | water and underwater programs. |
| `loadShadersEffects()` | glow and glow extraction programs. |
| `loadShadersInterface()` | UI, highlight, pathfinding, glow combine, occlusion, debug, normal debug, copy, reflection probe display/filtering, terrain bake, alpha mask. |
| `loadShadersObject()` | object bump, alpha mask/no-color, impostor, object preview, physics preview. |
| `loadShadersAvatar()` | avatar and avatar eyeball programs. |
| `loadShadersDeferred()` | G-buffer, material, PBR, GLTF variants, terrain, trees, lights, shadows, sky, haze, alpha, fullbright, emissive, post, DoF, exposure, luminance, FXAA, SMAA, CAS, RLV sphere, Mare velocity/TAA/NIS. |

## Mare-Specific Shader Areas

Mare additions currently visible in the shader map:

| area | shader handles/files | note |
|---|---|---|
| Motion vectors | `gDeferredVelocityProgram`, `gDeferredDynamicVelocityProgram`, `gDeferredAvatarVelocityProgram`; `velocity*.glsl`, `dynamicVelocity*.glsl`, `avatarVelocity*.glsl` | Feeds TAA/FSR2-style temporal paths. |
| TAA | `gDeferredTAAProgram`, `gDeferredTAACopyProgram`; `mareUpscale*.glsl`, `mareCopyF.glsl` | Fragment-shader temporal accumulation/copy path. |
| NIS | `gDeferredNISProgram`; `mareNISF.glsl` | Fragment-shader sharpening path. |
| FSR2 compute assets | `class1/deferred/fsr2/*.comp.glsl` | Compute shader assets used by `marefsr2upscaler.cpp`; build-disabled on Darwin by `MARE_ENABLE_FSR2=0`. |

Risk: the FSR2 compute assets require OpenGL compute/image load-store APIs not
available through macOS OpenGL. Keeping FSR2 excluded on Darwin remains the
right containment boundary until a compatible non-compute fallback exists.

## GLTF Variant Model

`LLGLSLShader` stores up to 16 GLTF variants. The bits encode:

- alpha blend
- rigged
- unlit
- multi-UV

`make_gltf_variants()` builds those variants from one base shader by applying
permutation defines. Alpha-blend variants also alter feature flags for
atmospherics, gamma, shadows, deferred utilities, and reflection probes.

Risk: GLTF variant creation multiplies one base shader into many runtime
programs. Any change to shared GLTF feature flags can affect static, rigged,
lit, unlit, single-UV, multi-UV, opaque, and alpha-blend paths together.

## Shader Cache

`LLShaderMgr` can use OpenGL program binaries when:

- `RenderShaderCacheEnabled` is true.
- GL version is at least 4.09.
- It is safe for the current viewer instance.

The cache key is based on `LLGLSLShader::hash()`, which depends on shader files,
defines, and related inputs. Metadata is stored under the viewer cache directory
as `shader_cache/shaderdata.llsd`.

Observed runtime note from the baseline smoke test: shader cache metadata save
can fail in the local cache path. This is a runtime/cache issue, not a shader
source mapping issue.

## High-Risk Boundaries

- `LLShaderMgr::attachShaderFeatures()` attachment order is explicitly called
  important in comments. Do not reorder early.
- `LLShaderMgr::loadShaderFile()` injects GLSL version, defines, G-buffer
  constants, indexed texture lookup code, and platform workarounds. This is not
  just file I/O.
- `LLGLSLShader::createShader()` can recursively retry at lower shader levels.
- `LLViewerShaderMgr::setShaders()` releases GL buffers before reload and
  recreates them afterward. Shader loading is tied to pipeline resource
  lifecycle.
- Global shader handles are shared by draw pools, pipeline passes, UI preview
  paths, reflection code, water, avatars, GLTF, and post-processing.
- The shader class fallback model means a file missing in `class3` can silently
  resolve to `class2` or `class1`.
- Runtime settings alter global defines and feature flags. The loaded shader
  graph is setting-dependent.

## Areas Not To Modify First

- `LLShaderMgr::attachShaderFeatures()`.
- `LLShaderMgr::loadShaderFile()`.
- `LLGLSLShader::createShader()`, `mapUniforms()`, `bind()`, and texture channel
  assignment.
- `LLViewerShaderMgr::setShaders()` load order.
- `loadBasicShaders()` global define construction.
- `loadShadersDeferred()` until draw-pass and render-target ownership are more
  completely mapped.
- Mare velocity/TAA/NIS/FSR2 shader paths until the platform boundary document
  exists.

## Small Follow-Up Tasks

- Add `docs/architecture/07-ui-render-boundaries.md` for preview, map, HUD, and
  core UI rendering touchpoints.
- Add `docs/architecture/08-platform-opengl.md` for Darwin, Windows, SDL, and
  Mesa/headless OpenGL context glue.
- Consider a later generator update that exports shader file family counts and
  `mShaderFiles` references automatically.
