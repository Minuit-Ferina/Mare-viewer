# LLGLSLShader Remaining Direct Call Decision

Branch: `phase3`
Scope: `indra/llrender/llglslshader.cpp`

## Current State

After the phase 3 `LLGLSLShader` wrapper packets, the generated inventory
reports:
- `indra/llrender/llglslshader.cpp`: 16 active direct `gl*` references.
- `indra/llrender/llglcontainment.cpp`: 97 active direct `gl*` references.

The remaining `LLGLSLShader` direct calls are no longer the easy
query/setter-shaped wrappers already handled.

## Remaining Call Families

### Program Lifecycle

Remaining calls:
- `glCreateProgram`
- `glGetAttachedShaders`
- `glDetachShader`
- `glIsShader`
- `glDeleteShader`
- `glDeleteProgram`

Decision:
- Leave direct for now.

Reason:
- These calls define shader object ownership, reload behavior, and deletion
  order. They need a lifecycle contract before routing.

### Shader Attach

Remaining calls:
- `glAttachShader`

Decision:
- Leave direct for now.

Reason:
- Attach order and shader object ownership are coupled to
  `LLShaderMgr::mVertexShaderObjects`, `mFragmentShaderObjects`, and the manual
  attach path.

### Attribute Binding Before Link

Remaining call:
- `glBindAttribLocation`

Decision:
- Leave direct for now.

Reason:
- It affects link-time attribute layout and should be reviewed with shader link
  behavior, not with runtime setters.

### Uniform Block Binding

Remaining call:
- `glUniformBlockBinding`

Decision:
- Leave direct for now.

Reason:
- It writes program UBO binding policy and is tied to the Apple-compatible UBO
  setup path. It needs a small contract before routing.

### Program Binding

Remaining calls:
- `glUseProgram`

Decision:
- Leave direct for now.

Reason:
- Program bind/unbind is global OpenGL state and directly affects draw order.
  It should be handled separately from wrapper-only setter packets.

### Inactive Debug Include Block

Remaining calls:
- `glGetShaderiv`
- `glGetProgramInfoLog`

Decision:
- Leave direct for now.

Reason:
- The block is compile-time disabled by `DEBUG_SHADER_INCLUDES`.

## Recommended Next Steps

Small next tasks:
- document `LLGLSLShader` program lifecycle ownership
- document shader attach ownership and object sharing
- document program bind/unbind state assumptions
- document UBO binding policy separately

Only after those contracts are written should any remaining direct calls be
routed through `llglcontainment.*`.

