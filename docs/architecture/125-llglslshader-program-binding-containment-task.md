# LLGLSLShader Program Binding Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route only program bind/unbind calls in `LLGLSLShader` through
`llglcontainment.*`.

Included raw OpenGL family:
- `glUseProgram`

Included callsites:
- `LLGLSLShader::bind()`
- `LLGLSLShader::unbind()`

## Non-Scope

Do not route in this packet:
- shader lifecycle calls
- shader attach/detach/delete calls
- inactive `DEBUG_SHADER_INCLUDES` code

Do not change:
- `gGL.flush()` ordering
- `LLVertexBuffer::unbind()` ordering
- profile query reads
- `sCurBoundShader` assignment
- `sCurBoundShaderPtr` assignment
- `placeProfileQuery()`
- uniform dirty update logic

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- current shader tracking
- redundant-bind avoidance
- profile query timing around bind/unbind
- vertex buffer client array setup
- dirty uniform update policy

`llglcontainment.*` owns only:
- direct `glUseProgram(...)` call-through.

## Ordering Contract

The source patch must preserve:

1. flush renderer before changing program.
2. read pending profile query before binding a different program.
3. unbind vertex buffers before changing program.
4. call program bind/unbind at the same point as before.
5. update current-program globals after binding.
6. place a profile query after binding.

## Risk

Risk: medium to high.

Why:
- Program binding is global OpenGL state.
- This patch is still a call-through wrapper only, but any ordering change would
  be visible to draw pools and UI rendering.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- recommended at a later checkpoint because this is global state, but not
  required for the isolated wrapper commit.

