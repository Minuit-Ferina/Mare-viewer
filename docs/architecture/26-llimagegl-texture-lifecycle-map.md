# LLImageGL Texture Lifecycle Map

This document starts the phase 2 review of `LLImageGL`. It is documentation
only and does not change source behavior.

Related files:

- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llimagegl.h`

## Inventory

Generated inventory row:

- file: `indra/llrender/llimagegl.cpp`
- category: `render.legacy_low_level`
- lines: 2651
- likely `gl*` calls: 71
- raw `gl*` references: 84
- `gGL` references: 32
- `LLGL` references: 21
- `LLImageGL` references: 123

Local function-frequency scan of likely OpenGL names:

| call family | count |
|---|---:|
| `glPixelStorei` | 12 |
| `glTexParameteri` | 5 |
| `glGetTexLevelParameteriv` | 5 |
| `glTexImage2D` | 4 |
| `glBindBuffer` | 4 |
| `glTexSubImage2D` | 3 |
| `glTexParameteriv` | 3 |
| `glGetError` | 3 |
| `glGenerateMipmap` | 3 |
| `glFlush` | 3 |
| `glGetTexImage` | 2 |
| `glGetIntegerv` | 2 |
| `glGenTextures` | 2 |
| `glGenBuffers` | 2 |
| `glFenceSync` | 2 |
| `glDeleteSync` | 2 |
| `glCopyTexSubImage2D` | 2 |
| `glCompressedTexImage2D` | 2 |
| single-call families | 10 |

Note: a simple text scan also sees a comment reference to `glSetSubImage2D`.
That is not a direct OpenGL callsite.

## Primary Responsibilities

`LLImageGL` owns several overlapping responsibilities:

- GL texture name lifecycle
- texture memory accounting
- full texture image allocation and upload
- partial texture updates
- compressed and uncompressed texture handling
- mipmap creation and generation
- texture readback
- GPU-assisted texture scale-down
- delayed deletion of texture names
- thread handoff for texture upload completion
- texture option dirtiness and residency checks

This is wider than `LLRenderTarget`. Treat it as several smaller contracts, not
one patch.

## Callsite Families

### Texture Name Lifecycle

Likely owner methods:

- `LLImageGL::generateTextures(...)`
- `LLImageGL::deleteTextures(...)`
- `LLImageGL::updateClass()`
- `LLImageGL::destroyGLTexture()`
- `LLImageGL::syncTexName(...)`
- `LLImageGL::createGLTexture(...)`

OpenGL families:

- `glGenTextures`
- `glDeleteTextures`

Observed behavior:

- texture name generation uses a thread-local pool
- deletion is delayed through `sFreeList`
- old texture names may be deleted immediately on the main thread or through
  a sync callback path

Risk:

- high, because texture names are shared across upload, deletion delay,
  memory accounting, and thread handoff

Priority:

- P0 for documentation before any source cleanup

### Full Texture Allocation And Upload

Likely owner methods:

- `LLImageGL::setImage(...)`
- `LLImageGL::setManualImage(...)`
- `LLImageGL::createGLTexture(...)`

OpenGL families:

- `glTexImage2D`
- `glCompressedTexImage2D`
- `glPixelStorei`
- `glTexParameteri`
- `glTexParameteriv`
- `glGenerateMipmap`

Observed behavior:

- compressed textures use `glCompressedTexImage2D`
- uncompressed textures route through `setManualImage(...)`
- core profile can convert deprecated formats through swizzle state
- `mFormatSwapBytes` temporarily changes `GL_UNPACK_SWAP_BYTES`
- automatic mipmap generation differs between core profile and legacy profile
- manual mipmap generation uploads multiple levels

Risk:

- high, because pixel store and mipmap state are global OpenGL state

Priority:

- P0 for documentation
- source cleanup only after pixel store and mipmap ownership are separately
  documented

### Partial Texture Updates

Likely owner methods:

- `LLImageGL::setSubImage(...)`
- `sub_image_lines(...)`

OpenGL families:

- `glTexSubImage2D`
- `glPixelStorei`

Observed behavior:

- full-image updates can redirect back to `setImage(...)`
- partial updates reject mipmapped images
- `GL_UNPACK_ROW_LENGTH` is set before upload and reset afterward
- `sub_image_lines(...)` can split updates into smaller batches depending on
  platform and driver heuristics

Risk:

- high, because row length and swap-byte state can leak into later uploads

Priority:

- P0 for documentation

### Framebuffer Copy Into Texture

Likely owner methods:

- `LLImageGL::setSubImageFromFrameBuffer(...)`
- `LLImageGL::scaleDown(...)`

OpenGL families:

- `glCopyTexSubImage2D`
- `glViewport`
- `glDrawArrays`

Observed behavior:

- `setSubImageFromFrameBuffer(...)` copies from the current framebuffer into
  the bound texture
- `scaleDown(...)` can use an FBO-style path with viewport change and full
  screen triangle draw

Risk:

- high, because this crosses texture ownership and current framebuffer state

Priority:

- P1 documentation before source cleanup

### Texture Readback

Likely owner methods:

- `LLImageGL::readBackRaw(...)`
- `LLImageGL::scaleDown(...)`

OpenGL families:

- `glGetTexLevelParameteriv`
- `glGetCompressedTexImage`
- `glGetTexImage`
- `glGetError`
- `glBindBuffer`
- `glBufferData`

Observed behavior:

- `readBackRaw(...)` queries mip size and compression state
- compressed and uncompressed readback paths allocate different data sizes
- GL errors before and after readback are logged
- `scaleDown(...)` can use a scratch PBO for readback and re-upload

Risk:

- medium-high, because readback consumes GL error state and can involve PBO
  binding

Priority:

- P1 documentation before source cleanup

### PBO And Sync Helpers

Likely owner methods:

- `LLImageGL::allocateConversionBuffer()`
- `LLImageGL::cleanupClass()`
- `LLImageGL::scaleDown(...)`
- `LLImageGL::syncToMainThread(...)`

OpenGL families:

- `glGenBuffers`
- `glDeleteBuffers`
- `glBindBuffer`
- `glBufferData`
- `glFenceSync`
- `glClientWaitSync`
- `glWaitSync`
- `glDeleteSync`
- `glFlush`

Observed behavior:

- `sScratchPBO` is shared class state
- sync behavior differs for NVIDIA versus the other path
- the non-NVIDIA path posts a main-thread wait/delete callback

Risk:

- high, because this combines global GL objects, GPU sync, and work queues

Priority:

- P1 documentation, P2 source cleanup

### Debug Queries And Residency

Likely owner methods:

- `LLImageGL::checkTexSize(...)`
- `LLImageGL::getIsResident(...)`

OpenGL families:

- `glGetIntegerv`
- `glGetTexLevelParameteriv`
- `glAreTexturesResident`

Observed behavior:

- `checkTexSize(...)` compares expected dimensions against GL texture state
- residency checking is optional and updates local cached state

Risk:

- medium, mostly debug and diagnostic state

Priority:

- P2

## Areas Not To Touch First

Do not start with:

- `LLImageGL::setImage(...)`
- `LLImageGL::setManualImage(...)`
- `LLImageGL::createGLTexture(...)`
- `LLImageGL::syncToMainThread(...)`
- `LLImageGL::scaleDown(...)`

Reason:

- these functions mix multiple OpenGL state families and ownership contracts
- changing them before smaller contracts are documented would make review
  difficult

## Safer Next Documents

Recommended next phase 2 documents:

1. `LLImageGL` texture name lifetime contract:
   `generateTextures`, `deleteTextures`, `updateClass`, `syncTexName`,
   `destroyGLTexture`.
2. `LLImageGL` pixel store contract:
   `GL_UNPACK_SWAP_BYTES`, `GL_UNPACK_ROW_LENGTH`, and reset requirements.
3. `LLImageGL` readback contract:
   `readBackRaw`, PBO readback in `scaleDown`, and GL error consumption.

Do not move any of this to `llglcontainment.*` yet. `LLImageGL` is still the
local owner.
