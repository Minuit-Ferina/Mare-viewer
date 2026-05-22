# LLGLSLShader Create Attach Containment Summary

Branch: `phase3`
Source commit: `228e2e8d0b`

## Scope Completed

Routed `LLGLSLShader` program creation and shader attachment call-throughs
through `llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call families:
- `glCreateProgram`
- `glAttachShader`

## Behavior Notes

`LLGLSLShader` still owns:
- program object storage
- zero-handle failure handling
- shader manager object lookup
- cached binary branches
- attach preconditions
- `stop_glerror()` ordering
- shader loading warnings

`llglcontainment.*` owns only the direct OpenGL call-through.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 12
  to 8.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 100 to 102.

## Remaining LLGLSLShader Direct Calls

Still direct:
- attached shader enumeration
- shader detach/delete
- program delete
- inactive `DEBUG_SHADER_INCLUDES` code

