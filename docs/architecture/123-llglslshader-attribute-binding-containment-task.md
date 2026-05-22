# LLGLSLShader Attribute Binding Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route only the reserved attribute binding call in
`LLGLSLShader::mapAttributes()` through `llglcontainment.*`.

Included raw OpenGL family:
- `glBindAttribLocation`

Included callsite:
- reserved attribute binding loop before `link()`

## Non-Scope

Do not route in this packet:
- `link()`
- program bind/unbind
- shader lifecycle calls
- shader attach/detach/delete calls
- inactive `DEBUG_SHADER_INCLUDES` code

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- `mUsingBinaryProgram` branch
- reserved attribute list iteration
- reserved attribute numeric locations
- link timing after all locations are bound
- attribute cache population after link

`llglcontainment.*` owns only:
- direct `glBindAttribLocation(...)` call-through.

## Ordering Contract

The source patch must preserve:

1. bind all reserved attribute names before `link()`.
2. call `link()` only after the full binding loop.
3. populate `mAttribute` only after link result is known.

## Risk

Risk: medium.

Why:
- It is a single wrapper move, but the call affects link-time attribute layout.
- The patch must not change the reserved attribute index/name mapping.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred for this isolated call-through move.

