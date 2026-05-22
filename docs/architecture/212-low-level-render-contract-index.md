# Low-Level Render Contract Index

Date: 2026-05-22

## Scope

This note indexes the low-level render contracts already written for
`llgl*`, `llrender*`, `llrendertarget*`, `llimagegl*`, and
`llvertexbuffer*`.

It does not make these files low-risk. It only records that the initial
"document contracts before source behavior changes" condition now has a
reviewable evidence trail.

## Contract Areas

### `LLGLContainment`

Primary docs:

- `docs/architecture/11-gl-containment-api-scope.md`
- `docs/architecture/182-llglcontainment-contract.md`
- `docs/architecture/183-gl-containment-guardrail.md`
- `docs/architecture/184-llglcontainment-wrapper-taxonomy.md`
- `docs/architecture/203-gl-header-boundary-guardrail.md`
- `docs/architecture/204-guardrail-review-after-header-boundary.md`

Current status:

- Runtime `gl*` calls are guarded so new callsites outside
  `indra/llrender/llglcontainment.cpp` fail the containment check.
- Runtime headers are guarded against direct `llgl.h` includes outside
  precompiled/prefix headers.
- Raw GL scalar type names in runtime headers are guarded and confined to
  `llglheaders.h`.

### `LLRenderTarget`

Primary docs:

- `docs/architecture/05-render-target-lifecycle.md`
- `docs/architecture/15-llrendertarget-viewport-contract.md`
- `docs/architecture/16-llrendertarget-fbo-contract.md`
- `docs/architecture/17-llrendertarget-buffer-routing-contract.md`
- `docs/architecture/18-llrendertarget-attachment-contract.md`
- `docs/architecture/19-llrendertarget-fbo-lifetime-contract.md`
- `docs/architecture/20-llrendertarget-mipmap-contract.md`
- `docs/architecture/21-llrendertarget-clear-contract.md`
- `docs/architecture/22-llrendertarget-framebuffer-status-contract.md`
- `docs/architecture/23-llrendertarget-texture-allocation-error-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/25-llrendertarget-phase2-review-summary.md`
- `docs/architecture/61-llrendertarget-phase3-completion-summary.md`
- `docs/architecture/194-llrendertarget-header-boundary-summary.md`

Current status:

- FBO, attachment, clear, mipmap, viewport, status, lifetime, and allocation
  behaviors are documented.
- Source containment has been split into small packets.
- Header boundary exposure has been narrowed.

Remaining risk:

- `LLRenderTarget` still owns local static FBO binding state and implicit
  target stack behavior. Future changes must name ordering and restore rules.

### `LLImageGL`

Primary docs:

- `docs/architecture/26-llimagegl-texture-lifecycle-map.md`
- `docs/architecture/27-llimagegl-texture-name-lifetime-contract.md`
- `docs/architecture/28-llimagegl-pixel-store-contract.md`
- `docs/architecture/29-llimagegl-readback-copy-contract.md`
- `docs/architecture/30-llimagegl-pbo-sync-contract.md`
- `docs/architecture/31-llimagegl-phase2-review-summary.md`
- `docs/architecture/32-llimagegl-upload-mipmap-parameter-contract.md`
- `docs/architecture/67-llimagegl-phase3-completion-summary.md`
- `docs/architecture/69-llimagegl-remaining-call-classification.md`
- `docs/architecture/71-llimagegl-wrapper-pure-completion-summary.md`
- `docs/architecture/80-llimagegl-scaledown-containment-summary.md`
- `docs/architecture/193-llgltexture-header-boundary-summary.md`

Current status:

- Texture name lifetime, pixel store, readback/copy, PBO sync, upload/mipmap,
  and scaledown paths are documented.
- Header-level GL exposure from texture headers has been reduced.

Remaining risk:

- Texture ownership crosses fetched textures, local textures, GLTF preview,
  render targets, UI previews, and cache/upload paths.

### `LLVertexBuffer`

Primary docs:

- `docs/architecture/33-llvertexbuffer-buffer-binding-update-contract.md`
- `docs/architecture/34-llvertexbuffer-phase2-review-summary.md`
- `docs/architecture/63-llvertexbuffer-buffer-name-containment-task.md`
- `docs/architecture/64-llvertexbuffer-remaining-containment-task.md`
- `docs/architecture/65-llvertexbuffer-phase3-completion-summary.md`
- `docs/architecture/195-llvertexbuffer-header-boundary-summary.md`

Current status:

- Buffer binding, update, name lifetime, and remaining call families are
  documented.
- Header-level GL type exposure has been reduced.

Remaining risk:

- Vertex buffer behavior is shared by draw pools, previews, UI helpers, and
  debug drawing. It should stay behavior-stable unless a task names an exact
  call family.

### `LLRender` And `LLTexUnit`

Primary docs:

- `docs/architecture/90-llrender-remaining-call-classification.md`
- `docs/architecture/91-llrender-call-family-map.md`
- `docs/architecture/100-llrender-set-line-width-containment-task.md`
- `docs/architecture/101-llrender-set-line-width-summary.md`
- `docs/architecture/102-llrender-blend-color-containment-task.md`
- `docs/architecture/103-llrender-blend-color-summary.md`
- `docs/architecture/104-llrender-global-init-containment-task.md`
- `docs/architecture/105-llrender-global-init-summary.md`
- `docs/architecture/106-lltexunit-binding-containment-task.md`
- `docs/architecture/107-lltexunit-binding-summary.md`
- `docs/architecture/187-llrender-header-boundary-summary.md`
- `docs/architecture/188-llrender2dutils-header-boundary-summary.md`

Current status:

- Immediate rendering state, line width, blend color, global init, and texture
  unit binding call families have been mapped and contained.
- Header exposure from common 2D render helper headers has been narrowed.

Remaining risk:

- `gGL` remains the high-level immediate rendering surface across world, UI,
  debug, preview, and draw-pool code. It is not just an OpenGL wrapper.

### `LLGLSLShader` And `LLShaderMgr`

Primary docs:

- `docs/architecture/06-shader-map.md`
- `docs/architecture/109-llglslshader-call-family-map.md`
- `docs/architecture/110-llglslshader-profile-query-containment-task.md`
- `docs/architecture/120-llglslshader-remaining-direct-call-decision.md`
- `docs/architecture/121-llglslshader-ubo-binding-containment-task.md`
- `docs/architecture/126-llglslshader-program-binding-summary.md`
- `docs/architecture/130-llglslshader-unload-lifecycle-summary.md`
- `docs/architecture/133-llshadermgr-containment-task.md`
- `docs/architecture/134-llshadermgr-containment-summary.md`
- `docs/architecture/189-llglslshader-header-boundary-summary.md`
- `docs/architecture/190-llshadermgr-header-boundary-summary.md`

Current status:

- Shader program/query/uniform/attribute/UBO/load-unload families have been
  mapped and contained in small packets.
- Shader header exposure has been narrowed.

Remaining risk:

- Shader manager changes can affect deferred, PBR, GLTF, terrain, avatar,
  post-processing, and upscaler paths together.

### `LLGL`, `LLGLStates`, And `LLGLHeaders`

Primary docs:

- `docs/architecture/177-llgl-boundary-containment-task.md`
- `docs/architecture/178-llgl-boundary-containment-summary.md`
- `docs/architecture/185-llglheaders-include-trim-summary.md`
- `docs/architecture/186-llglheaders-typeonly-trim-summary.md`
- `docs/architecture/196-llglstates-type-boundary-summary.md`
- `docs/architecture/201-llgl-header-type-boundary-summary.md`
- `docs/architecture/203-gl-header-boundary-guardrail.md`

Current status:

- `llgl.cpp` is treated as low-level loader/capability/state boundary code.
- `llglheaders.h` owns platform GL ABI declarations and function pointer names.
- `llgl.h` and `llglstates.h` expose viewer type aliases instead of raw GL
  scalar spellings where practical.

Remaining risk:

- Loader and platform declaration code is intentionally not hidden behind
  generic containment wrappers. It remains an explicit boundary exception.

## Interpretation

The low-level contract prerequisite is satisfied for starting narrowly scoped
future tasks.

This does not authorize broad edits. A future source task still needs:

- exact owner file;
- exact call family or behavior;
- current state owner;
- ordering and restore behavior;
- platform constraints;
- targeted build path;
- runtime smoke only when the behavior can affect visible rendering.
