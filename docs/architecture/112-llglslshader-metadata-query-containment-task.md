# LLGLSLShader Metadata Query Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route read-only shader metadata query calls from
`indra/llrender/llglslshader.cpp` through `llglcontainment.*`.

Included raw OpenGL families:
- `glGetError`
- `glGetUniformLocation`
- `glGetAttribLocation`
- `glGetProgramiv`
- `glGetActiveUniform`
- `glGetUniformBlockIndex`

Included callsites:
- unload-time Apple error drain
- reserved attribute location lookup
- active uniform introspection
- reserved uniform location lookup
- debug uniform location validation
- uniform block index lookup

## Non-Scope

Do not route:
- `glBindAttribLocation`
- `glUniformBlockBinding`
- `glUseProgram`
- `glUniform*`
- `glVertexAttrib*`
- shader program creation, attach, detach, or deletion
- inactive `DEBUG_SHADER_INCLUDES` code

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- attribute and uniform caches
- reserved uniform ordering policy
- texture channel assignment
- UBO binding policy
- debug validation decisions

`llglcontainment.*` owns only:
- the direct OpenGL read/query call-through
- viewer-to-GL count conversion where needed

## Risk

Risk: medium.

Why:
- Calls are read-only, but their results populate important shader caches.
- No ordering changes are allowed around `bind()`, `mapUniform(...)`, or
  texture channel assignment.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless a later packet touches binding, uniform writes, or shader
  lifecycle.

