# LLGLContainment Wrapper Taxonomy

Branch: `phase3`

## Purpose

This classifies the current `LLGLContainment` wrapper surface by future
renderer concern. It is not a proposal to split files yet. The goal is to make
the mechanical wrappers reviewable before any later design work.

Current snapshot:

- public declarations in `llglcontainment.h`: 162
- runtime `gl*` calls in `llglcontainment.cpp`: 160
- runtime `gl*` calls outside `llglcontainment.cpp`: 0

## Framebuffer And Render Targets

Wrappers:

- `bindReadWriteFramebuffer`, `bindFramebuffer`
- `getFramebufferStatus`, `getDrawFramebufferStatus`
- `setReadWriteFramebufferTexture2D`
- `setDrawBuffer`, `setReadBuffer`, `setDrawBuffers`
- `generateFramebuffers`, `deleteFramebuffers`

Likely owners:

- `LLRenderTarget`
- `pipeline.cpp`
- frame setup and post-processing code

Risk:

- high, because these calls affect render target binding, attachment state, and
  draw/read routing.

Future direction:

- Keep ownership in `LLRenderTarget` and frame orchestration code.
- Do not add implicit restore behavior in containment.

## Textures And Image Transfer

Wrappers:

- texture names and binding: `generateTextures`, `deleteTextures`,
  `createTextures`, `setActiveTexture`, `bindTexture`, `bindTextureUnit`,
  `bindImageTexture`
- storage and parameters: `setTextureStorage2D`,
  `setNamedTextureParameterInteger`, `setTextureParameterInteger`,
  `setTextureParameterFloat`, `setTextureParameterIntegerVector`
- upload and allocation: `setTextureImage2D`, `setTextureSubImage2D`,
  `setCompressedTextureImage2D`, `setTextureImage3D`,
  `setTextureSubImage3D`
- readback and copies: `readTextureImage`, `readCompressedTextureImage`,
  `readPixels`, `copyTextureSubImage2D`, `copyTextureSubImage3D`,
  `copyTextureImage2D`, `copyImageSubData`
- mipmaps and metadata: `generateTextureMipmap`,
  `getTextureLevelParameterInteger`, `areTexturesResident`
- legacy coordinate generation: `setTextureGenerationInteger`,
  `setTextureGenerationFloatVector`

Likely owners:

- `LLImageGL`
- `LLViewerTexture`
- GLTF material/preview code
- render target and post-processing paths

Risk:

- high, because texture calls mix resource lifetime, upload policy, readback,
  and legacy state.

Future direction:

- Keep texture name and upload policy in `LLImageGL`.
- Treat GLTF/image upscaler paths as callers, not containment owners.

## Buffers, Vertex Input, And Drawing

Wrappers:

- buffer objects: `generateBufferObjects`, `deleteBufferObjects`,
  `bindBufferObject`, `bindBufferBase`, `allocateBufferObjectStorage`,
  `updateBufferObjectSubData`, `getBufferObjectParameterInteger`
- vertex attributes: `enableVertexAttributeArray`,
  `disableVertexAttributeArray`, `setVertexAttributePointer`,
  `setIntegerVertexAttributePointer`, `setVertexAttribute4`,
  `setVertexAttributeVector4`
- legacy client arrays: `setVertexPointer`, `enableClientState`,
  `disableClientState`, `setTextureCoordinatePointer`
- draw entry points: `drawVertexBufferRange`, `drawVertexBufferArrays`,
  `drawElements`
- vertex array objects: `hasVertexArrayGenerator`, `generateVertexArrays`,
  `bindVertexArray`

Likely owners:

- `LLVertexBuffer`
- draw pools
- debug render paths

Risk:

- high, because draw order and vertex declarations are behavior-sensitive.

Future direction:

- Preserve existing `LLVertexBuffer` ownership.
- Do not combine draw calls or hide index/array selection inside containment.

## Shaders, Programs, And Uniforms

Wrappers:

- program and shader lifecycle: `createProgram`, `deleteProgram`,
  `createShader`, `deleteShader`, `attachShader`, `detachShader`,
  `compileShader`, `linkProgram`, `validateProgram`, `useProgram`
- reflection and metadata: `getUniformLocation`, `getAttributeLocation`,
  `getAttachedShaders`, `getShaderInfoLog`, `getProgramInfoLog`,
  `getShaderInteger`, `getProgramInteger`, `getActiveUniform`,
  `getUniformBlockIndex`
- binding and binary support: `bindAttributeLocation`, `bindUniformBlock`,
  `setProgramParameterInteger`, `setProgramBinary`, `getProgramBinary`
- uniforms: all `setUniform*` and `setUniformMatrix*` wrappers
- compute, non-Darwin only: `dispatchCompute`, `setMemoryBarrier`

Likely owners:

- `LLGLSLShader`
- `LLShaderMgr`
- GLTF and post-processing shader users
- FSR2 or other non-Darwin compute paths

Risk:

- medium to high. Most wrappers are mechanical, but shader lifecycle and
  program binding affect global renderer state.

Future direction:

- Keep shader ownership in shader classes.
- Keep Darwin compute exclusions explicit.

## State, Fixed Function, And Legacy OpenGL

Wrappers:

- generic state queries: `getInteger`, `getBoolean`, `getFloat`, `getString`,
  `getStringIndexed`, `getError`
- generic state changes: `enableCapability`, `disableCapability`,
  `isCapabilityEnabled`, `setHint`, `setPixelStoreInteger`
- matrix stack: `setMatrixMode`, `pushMatrix`, `popMatrix`
- attributes: `pushAttributeBits`, `pushClientAttributeBits`,
  `popClientAttributes`, `popAttributes`
- material and color: `setMaterialFloatVector`, `setMaterialInteger`,
  `setClearColor`, `setColorMask`, `setColorUnsignedByteVector`
- depth, culling, blend, viewport: `setDepthFunction`, `setDepthMask`,
  `setCullFace`, `setBlendFunction`, `setSeparateBlendFunction`,
  `setViewport`
- raster state: `setPolygonOffset`, `setPolygonMode`, `setScissorBox`,
  `setStencilFunction`, `setStencilMask`, `setStencilOperation`,
  `setLineWidth`, `setPointSize`, `clearBuffers`, `setClientActiveTexture`

Likely owners:

- `LLRender`
- `LLGLState` and related state guards
- draw pools
- debug overlays
- `pipeline.cpp`

Risk:

- highest long-term design risk. These wrappers touch the old global state
  model and fixed-function compatibility paths.

Future direction:

- Do not add new state policy here.
- Future cleanup should start by documenting ownership and restore semantics in
  the caller, not by making containment smarter.

## Queries, Sync, Debug, And Diagnostics

Wrappers:

- queries: `generateQueries`, `deleteQueries`, `beginQuery`, `endQuery`,
  `getQueryObjectUnsignedInteger`, `getQueryObjectUnsignedInteger64`
- sync: `createSyncObject`, `clientWaitSyncObject`,
  `clientWaitSyncObjectStatus`, `waitSyncObject`, `deleteSyncObject`,
  `flushCommands`, `finishCommands`
- debug: `setDebugMessageCallback`

Likely owners:

- occlusion query code
- debug and profiling paths
- image readback and synchronization paths

Risk:

- medium. They are mostly mechanical wrappers, but timing and synchronization
  changes can be hard to diagnose.

Future direction:

- Keep timing/query meaning in the caller.
- Do not add cross-frame queueing or implicit waits in containment.

## Unknown Or Transitional Surface

Wrapper:

- `getPhaseOneScope`

Reason:

- Historical marker from the initial phase 1 placeholder.

Future direction:

- Leave it until a cleanup task explicitly removes or renames it. It is harmless
  but no longer describes the full phase 3 state.
