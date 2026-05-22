# LLGLSLShader Create Attach Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route only shader program creation and shader attachment call-throughs from
`LLGLSLShader` through `llglcontainment.*`.

Included raw OpenGL families:
- `glCreateProgram`
- `glAttachShader`

Included callsites:
- `LLGLSLShader::createShader()`
- `LLGLSLShader::attachVertexObject(...)`
- `LLGLSLShader::attachFragmentObject(...)`
- `LLGLSLShader::attachObject(...)`

## Non-Scope

Do not route in this packet:
- shader detach/delete/program deletion calls
- attached-shader enumeration
- inactive `DEBUG_SHADER_INCLUDES` code

Do not change:
- shader source loading
- cached binary loading
- success/failure handling
- `mUsingBinaryProgram` branches
- `stop_glerror()` ordering
- debug include dumping

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- program object member storage
- zero-handle failure handling
- shader manager object lookup
- attach preconditions
- shader loading warnings

`llglcontainment.*` owns only:
- direct program creation call-through
- direct shader attach call-through

## Risk

Risk: medium.

Why:
- This still only moves call-through wrappers.
- The calls participate in shader lifecycle, so ownership and failure handling
  must remain in `LLGLSLShader`.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

