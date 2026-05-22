# LLGLSLShader Debug Include Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route the direct OpenGL calls inside the disabled `DEBUG_SHADER_INCLUDES` block
through existing `llglcontainment.*` helpers.

Included raw OpenGL families:
- `glGetShaderiv`
- `glGetProgramInfoLog`

Included callsite:
- `dumpAttachObject(...)` under `#if DEBUG_SHADER_INCLUDES`

## Non-Scope

Do not enable `DEBUG_SHADER_INCLUDES`.

Do not change:
- runtime shader loading
- attach ordering
- debug print formatting
- allocation/deallocation of `info_log`

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- whether the debug include block exists
- debug output formatting
- info log buffer lifetime

`llglcontainment.*` already owns:
- shader integer query call-through
- program info log call-through

## Risk

Risk: low for normal builds.

Why:
- The block is compile-time disabled by default.
- The patch only routes existing debug-only calls through existing wrappers.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

