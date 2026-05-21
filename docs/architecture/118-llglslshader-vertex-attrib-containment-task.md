# LLGLSLShader Vertex Attribute Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route only the two vertex attribute setter call-throughs from
`indra/llrender/llglslshader.cpp` through `llglcontainment.*`.

Included raw OpenGL families:
- `glVertexAttrib4f`
- `glVertexAttrib4fv`

Included callsites:
- `LLGLSLShader::vertexAttrib4f(...)`
- `LLGLSLShader::vertexAttrib4fv(...)`

## Non-Scope

Do not route in this packet:
- attribute binding before link
- program bind/unbind
- uniform block binding
- shader lifecycle
- inactive `DEBUG_SHADER_INCLUDES` code

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- attribute index lookup through `mAttribute`
- the existing `mAttribute[index] > 0` guard

`llglcontainment.*` owns only:
- direct vertex attribute write call-through.

## Risk

Risk: low.

Why:
- Two callsites.
- No attribute lookup or guard behavior changes.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred, because this packet only moves two call-through wrappers.

