# LLGLSLShader Call Family Map

Branch: `phase3`
Scope: `indra/llrender/llglslshader.cpp`

## Purpose

`LLGLSLShader` is now the highest-risk remaining `indra/llrender` source file
for direct OpenGL containment after the small `LLRender` and `LLTexUnit`
packets. This map groups its direct calls by intent before any source routing.

This document does not propose shader behavior changes. It only identifies
small wrapper candidates and areas that should remain contract-first.

## Call Families

### Profiling Query Objects

Probable calls:
- `glGenQueries`
- `glDeleteQueries`
- `glBeginQuery`
- `glEndQuery`
- `glGetQueryObjectui64v`

Current owner:
- `LLGLSLShader::placeProfileQuery(...)`
- `LLGLSLShader::readProfileQuery(...)`
- `LLGLSLShader::unloadInternal()`

Risk: low to medium.

Reason:
- The calls are confined to shader profiling counters.
- They do not participate in shader compile, link, bind, or uniform discovery.
- The wrapper surface can preserve exact ordering and data flow.

Priority:
- First source packet.

### Program And Shader Lifetime

Probable calls:
- `glCreateProgram`
- `glGetAttachedShaders`
- `glDetachShader`
- `glIsShader`
- `glDeleteShader`
- `glDeleteProgram`
- `glAttachShader`

Current owner:
- `LLGLSLShader::createShader()`
- `LLGLSLShader::unloadInternal()`
- attach helpers.

Risk: high.

Reason:
- These calls define shader object ownership and reload behavior.
- Mistakes can leak shader objects, delete shared objects too early, or break
  program relink paths.

Priority:
- Contract-first only. Do not route until object ownership is documented.

### Program Introspection

Probable calls:
- `glGetProgramiv`
- `glGetActiveUniform`
- `glGetUniformLocation`
- `glGetAttribLocation`
- `glGetUniformBlockIndex`

Current owner:
- uniform and attribute mapping during shader setup.

Risk: medium to high.

Reason:
- These calls populate cached maps used by texture channel assignment,
  reserved uniforms, and debug validation.
- They are mostly query-style calls, but their results drive later runtime
  writes.

Priority:
- Contract-first. Prefer a small read-only query packet after profiling
  queries if the cached ownership is documented.

### Program Binding

Probable calls:
- `glUseProgram`

Current owner:
- shader bind and unbind paths.

Risk: high.

Reason:
- Program binding is central global OpenGL state.
- Ordering is visible to draw pools, UI rendering, and post-processing.

Priority:
- Defer until binding side effects are mapped.

### Uniform Writes

Probable calls:
- `glUniform1i`
- `glUniform1f`
- `glUniform2f`
- `glUniform3f`
- `glUniform4f`
- `glUniform1iv`
- `glUniform4iv`
- `glUniform1fv`
- `glUniform2fv`
- `glUniform3fv`
- `glUniform4fv`
- `glUniform4uiv`
- `glUniformMatrix*`

Current owner:
- scalar, vector, array, and matrix uniform setters.

Risk: high.

Reason:
- These calls are numerous, performance-sensitive, and tied to caching and
  dirty-state logic.
- They are wrapper-shaped but touch a large shader API surface.

Priority:
- Defer until smaller query and lifecycle packets prove the containment shape.

### Vertex Attribute Writes

Probable calls:
- `glVertexAttrib4f`
- `glVertexAttrib4fv`

Current owner:
- attribute setter helpers.

Risk: medium.

Reason:
- Small call count, but still modifies currently bound program attribute state.

Priority:
- Later small packet, after program binding is better documented.

### Inactive Debug Include Block

Probable calls:
- `glGetShaderiv`
- `glGetProgramInfoLog`

Current owner:
- `dumpAttachObject(...)` under `#if DEBUG_SHADER_INCLUDES`.

Risk: unknown.

Reason:
- The block is compile-time disabled.
- It also contains code that should not be disturbed during active containment
  packets.

Priority:
- Leave direct for now.

## Recommended Next Packet

Route only profiling query object calls through `llglcontainment.*`.

Expected source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Verification:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

