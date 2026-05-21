# LLGLSLShader Public Uniform Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route the public `LLGLSLShader::uniform*` setter call-throughs from
`indra/llrender/llglslshader.cpp` through `llglcontainment.*`.

Included raw OpenGL families:
- scalar integer uniforms
- scalar float uniforms
- float vector uniforms
- integer vector uniforms
- unsigned integer vector uniforms
- matrix uniforms

Included function groups:
- index-based `uniform*` setters
- hashed-name `uniform*` setters
- `fastUniform1f(...)`

## Non-Scope

Do not route in this packet:
- `LLGLSLShader::mapUniformTextureChannel(...)` calls already handled
- `glUniformBlockBinding`
- `glUseProgram`
- `glVertexAttrib*`
- shader program lifecycle calls
- shader attach/detach/delete calls
- attribute binding before link

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- bound-shader assertions
- uniform index bounds checks
- cached value comparisons
- `mValue` updates
- stop-gl-error ordering around matrix updates

`llglcontainment.*` owns only:
- the direct OpenGL uniform write call-through
- count and transpose type conversion

## Risk

Risk: medium.

Why:
- The change is wrapper-pure, but touches many hot setter paths.
- Any altered count, vector width, or matrix API would be visible immediately.

Guardrail:
- Preserve each existing OpenGL function exactly. In particular, existing
  `uniform4iv(U32 index, ...)` still calls the one-component integer vector
  API and must not be changed to the four-component API in this packet.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless later binding/lifecycle packets are combined with this work.

