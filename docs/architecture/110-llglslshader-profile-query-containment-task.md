# LLGLSLShader Profile Query Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route only GPU profiling query OpenGL calls from
`indra/llrender/llglslshader.cpp` through `llglcontainment.*`.

Included callsites:
- `LLGLSLShader::placeProfileQuery(...)`
- `LLGLSLShader::readProfileQuery(...)`
- `LLGLSLShader::unloadInternal()` query deletion only

Included raw OpenGL families:
- query name generation
- query name deletion
- query begin/end
- query unsigned 64-bit result reads

## Non-Scope

Do not touch:
- shader program creation
- shader attach/detach/delete behavior
- program binding
- uniform discovery
- uniform writes
- vertex attribute writes
- inactive `DEBUG_SHADER_INCLUDES` code

## Current Ownership

`LLGLSLShader` owns:
- whether profiling is enabled
- whether runtime profiling is active
- pending query state
- accumulated timing, sample, triangle, and bind counters
- query object member fields

`llglcontainment.*` should own only:
- the direct OpenGL call-through wrapper
- basic type conversions from viewer integer types to GL call parameters

## Ordering Contract

The source patch must preserve this ordering:

1. generate query object names before first use
2. begin timer query
3. optionally begin sample and primitive queries
4. end timer query
5. optionally end sample and primitive queries
6. optionally poll availability for runtime profiling
7. read timer result
8. optionally read sample and primitive results
9. delete query object names during unload

## Risk

Risk: low to medium.

Why:
- This does not alter shader program state or uniform state.
- Query calls are already grouped into a small profiling path.
- The only API nuance is preserving `GLuint64` result storage exactly.

Residual risk:
- Query calls still depend on the current GL context and profiling extension
  availability.
- The source packet must not add fallback behavior or suppress errors.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Deferred:
- runtime smoke, unless later source packets touch shader binding or uniform
  behavior.

