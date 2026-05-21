# LLGLSLShader Texture Channel Uniform Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route only the uniform writes used by
`LLGLSLShader::mapUniformTextureChannel(...)` through `llglcontainment.*`.

Included raw OpenGL families:
- `glUniform1i`
- `glUniform1iv`

Included behavior:
- single sampler texture channel assignment
- sampler array texture channel assignment

## Non-Scope

Do not route in this packet:
- public `LLGLSLShader::uniform*` setter methods
- matrix uniform setters
- unsigned integer uniform setters
- vertex attribute setters
- `glUniformBlockBinding`

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- sampler detection
- texture channel allocation
- sampler array clamping to 16 channels
- `mActiveTextureChannels`

`llglcontainment.*` owns only the direct uniform write call-through.

## Risk

Risk: low to medium.

Why:
- The callsites are small and already isolated.
- They do write currently bound program state, but the surrounding bind and
  texture-channel allocation order is unchanged.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred, because this packet only moves call-through wrappers and does not
  alter texture-channel policy.

