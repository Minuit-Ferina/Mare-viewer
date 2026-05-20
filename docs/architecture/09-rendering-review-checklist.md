# Rendering Review Checklist

This checklist is for new rendering-related work during phase 1. It is a
documentation gate only. It does not change build behavior or runtime behavior.

Use it for any patch that touches rendering, render-adjacent UI, platform GL
setup, shaders, textures, render targets, or viewer display code.

## Required Summary In Reviews

Every rendering-related patch should state:

- files changed
- runtime behavior intentionally changed, if any
- whether direct OpenGL calls are added
- whether `gGL`, `LLGL`, `LLRenderTarget`, `LLImageGL`, `LLViewerTexture`,
  `LLGLSLShader`, or `LLPipeline` usage is added or changed
- whether UI code now requests or owns rendering state
- platform assumptions: Darwin, Windows, Linux/SDL, Mesa/headless
- verification performed

If behavior is intended to be unchanged, say that explicitly.

## Direct OpenGL Calls

Default rule: do not add new direct `gl*` calls outside `indra/llrender/`.

If a new direct `gl*` call outside `indra/llrender/` is proposed, the review
must include:

- exact function name, for example `glClear`, `glViewport`, or
  `glBindFramebuffer`
- file and function where it is added
- why existing wrappers or local helpers are insufficient
- why the call cannot live in `indra/llrender/`
- state affected by the call
- expected caller ordering
- restore or cleanup behavior, if applicable
- platform compatibility, especially Darwin OpenGL 4.1 limits
- a test or smoke-test plan

Do not accept "temporary" direct OpenGL calls without an issue or follow-up task
that says how they will be contained.

## `gGL` And `LLGL`

Adding or changing `gGL` or `LLGL*` usage should answer:

- Does the call affect global render state?
- Is the state scoped by an existing RAII helper?
- Can the state leak across draw pools, UI drawing, previews, or post passes?
- Does the code assume a specific matrix mode, texture unit, blend mode, cull
  mode, depth mode, color mask, stencil state, or scissor state?
- Does the call occur inside UI traversal or a render pass?

High-risk examples:

- `gGL.matrixMode()`
- `gGL.pushMatrix()` / `gGL.popMatrix()`
- `gGL.getTexUnit(0)->bind*()`
- `gGL.setSceneBlendType()`
- `gGL.flush()`
- `LLGLEnable`, `LLGLDisable`, `LLGLDepthTest`, `LLGLSUIDefault`

## Render Targets

For `LLRenderTarget` changes, state:

- owner of the target
- allocation size and format
- depth ownership or shared depth target
- bind and flush order
- clear behavior
- texture readback or sampling path
- resize behavior
- release behavior
- whether UI previews or post-processing depend on it

Do not change these early without a focused task:

- `LLRenderTarget::allocate()`
- `LLRenderTarget::bindTarget()`
- `LLRenderTarget::flush()`
- `LLRenderTarget::shareDepthBuffer()`
- `LLPipeline` render target pack ownership
- `LLViewerDynamicTexture::updateAllInstances()`

## Shaders

For shader manager or shader asset changes, state:

- shader files added, removed, or changed
- shader class and family, for example `class1/deferred`
- loader path in `LLViewerShaderMgr`
- feature flags changed in `LLShaderFeatures`
- uniforms or attributes added
- fallback behavior across shader classes
- platform capability assumptions
- whether shader cache keys are affected

High-risk areas:

- `LLShaderMgr::attachShaderFeatures()`
- `LLShaderMgr::loadShaderFile()`
- `LLGLSLShader::createShader()`
- `LLViewerShaderMgr::setShaders()`
- GLTF shader variant creation
- MARE TAA/NIS/FSR2 shader paths

## Textures And Images

For texture changes, state:

- owner: `LLImageGL`, `LLViewerTexture`, fetched texture, local texture, render
  target texture, or temporary GL texture
- upload path
- sampling path
- lifetime and release behavior
- format and filtering changes
- interaction with GLTF materials, terrain, previews, or UI controls

Do not mix texture lifetime changes with UI behavior changes in the same patch
unless the task explicitly requires it.

## UI And Rendering Boundaries

If the patch touches UI files that render content, answer:

- Is this ordinary UI drawing, preview rendering, map rendering, or world-scene
  rendering inside a UI control?
- Does it depend on `LLViewerDynamicTexture`?
- Does it bind or sample viewer textures?
- Does it change matrix, scissor, blend, cull, depth, stencil, or viewport
  state?
- Does it call into `gPipeline` or use pipeline-owned render targets?

High-risk UI/render boundary files:

- `indra/newview/lldynamictexture.cpp`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llgltfmaterialpreviewmgr.cpp`
- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llsnapshotlivepreview.cpp`
- `indra/newview/llnetmap.cpp`
- `indra/newview/llworldmapview.cpp`
- `indra/llui/lllocalcliprect.cpp`
- `indra/llui/llview.cpp`
- `indra/llui/lltextbase.cpp`

## Platform OpenGL

Platform OpenGL work must state:

- target platform
- context creation path affected
- swap path affected
- requested GL version or profile
- fallback behavior
- whether Darwin OpenGL 4.1 is sufficient
- whether Mesa/headless is affected

Do not use platform files as the first containment layer for renderer behavior.
Treat `indra/llwindow/` as context/window glue.

## FSR2 And Upscalers

FSR2-specific work must state:

- whether `MARE_ENABLE_FSR2` is required
- whether Darwin remains build-disabled
- compute shader files used
- raw OpenGL compute/image calls used
- fallback path when FSR2 is unavailable
- interaction with TAA, NIS, motion vectors, depth, and display-resolution
  render targets

Do not enable FSR2 on Darwin without a non-compute fallback or a separate
platform capability plan.

## Patch Size Rules

Keep rendering patches small:

- one boundary per patch
- one behavior change per patch
- no file moves during phase 1
- no broad wrapper conversions
- no global search-and-replace of OpenGL calls
- no Vulkan preparation code mixed with OpenGL containment

Prefer documentation and inventory updates before behavior changes.

## Verification

Minimum verification should match the risk:

| change type | minimum verification |
|---|---|
| docs-only | `git diff --check` |
| build-system only | configure or targeted build of affected target |
| low-level render source | targeted compile plus app launch smoke test |
| render target or pipeline behavior | app launch, login screen, one fixed-scene visual/FPS check |
| UI/render boundary | open affected floater/control and check visual output |
| shader changes | shader compile/load log check and visual smoke test |
| platform GL | app launch on affected platform or clearly documented missing coverage |

If verification cannot be run, the review should say why and what remains
untested.

## Reviewer Stop Points

Ask for a smaller patch when:

- source files are moved
- a patch mixes docs, source behavior, and platform glue without necessity
- a new abstraction wraps many existing OpenGL calls mechanically
- a direct `gl*` call is added outside `indra/llrender/` without justification
- UI rendering, render targets, and shader behavior are changed together
- Darwin FSR2 behavior changes without a platform capability plan
- behavior is claimed unchanged but no verification is given

## Related Maps

- `docs/architecture/03-opengl-debt.md`
- `docs/architecture/04-gl-callsite-inventory.md`
- `docs/architecture/05-render-target-lifecycle.md`
- `docs/architecture/06-shader-map.md`
- `docs/architecture/07-ui-render-boundaries.md`
- `docs/architecture/08-platform-opengl.md`
