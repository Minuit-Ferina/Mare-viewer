# LLGLSLShader Unload Lifecycle Containment Summary

Branch: `phase3`
Source commit: `45c3db41fb`

## Scope Completed

Routed unload-time shader/program lifecycle call-throughs from
`LLGLSLShader::unloadInternal()` through `llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call families:
- `glGetAttachedShaders`
- `glDetachShader`
- `glIsShader`
- `glDeleteShader`
- `glDeleteProgram`

## Behavior Notes

`LLGLSLShader` still owns:
- unload sequencing
- attached shader temporary storage
- detach loop order
- delete loop order
- program object reset
- query object cleanup after program cleanup

`llglcontainment.*` owns only the direct OpenGL call-through wrappers.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` references reduced
  from 8 to 3.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` references
  increased from 102 to 107.

## Remaining LLGLSLShader Direct References

Remaining references:
- one comment mentioning `glAttachShader`
- two calls inside the disabled `DEBUG_SHADER_INCLUDES` block

No active runtime `LLGLSLShader` OpenGL calls remain outside
`llglcontainment.*`.

