# LLGLSLShader Unload Lifecycle Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route only unload-time shader/program lifecycle call-throughs from
`LLGLSLShader::unloadInternal()` through `llglcontainment.*`.

Included raw OpenGL families:
- `glGetAttachedShaders`
- `glDetachShader`
- `glIsShader`
- `glDeleteShader`
- `glDeleteProgram`

## Non-Scope

Do not route in this packet:
- program creation
- shader attachment
- inactive `DEBUG_SHADER_INCLUDES` code

Do not change:
- `sInstances.erase(this)`
- cache clearing
- stop-gl-error ordering
- attached shader enumeration size
- detach loop order
- delete loop order
- `mProgramObject = 0`

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- unload lifecycle sequencing
- attached shader temporary storage
- program object member reset
- query object cleanup after program cleanup

`llglcontainment.*` owns only:
- direct OpenGL lifecycle call-through wrappers.

## Risk

Risk: medium.

Why:
- This remains wrapper-only, but it touches object lifetime cleanup.
- Any ordering change could leak or delete shader objects incorrectly.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

