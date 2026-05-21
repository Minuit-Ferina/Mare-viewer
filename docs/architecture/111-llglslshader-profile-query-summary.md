# LLGLSLShader Profile Query Containment Summary

Branch: `phase3`
Source commit: `e1015bc8fd`

## Scope Completed

Routed only `LLGLSLShader` GPU profiling query calls through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call families:
- `glGenQueries`
- `glDeleteQueries`
- `glBeginQuery`
- `glEndQuery`
- `glGetQueryObjectui64v`

## Behavior Notes

No profiling policy moved out of `LLGLSLShader`.

`LLGLSLShader` still owns:
- query member fields
- pending profile state
- runtime versus non-runtime profiling branches
- timing, sample, primitive, triangle, and bind accumulation

`llglcontainment.*` now owns only the direct OpenGL call-through for the query
operations.

The unsigned 64-bit query read wrapper keeps `GLuint64` local to
`llglcontainment.cpp` and returns the result through viewer type `U64`.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

Runtime smoke:
- deferred, because this packet only moved query call-through wrappers and did
  not alter shader binding, shader lifecycle, uniform writes, or render order.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 80
  to 65.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 69 to 74.

## Remaining LLGLSLShader Risk Areas

Still direct and intentionally not handled here:
- shader program creation and deletion
- shader attach/detach/delete paths
- program binding
- uniform and attribute discovery
- uniform writes
- vertex attribute writes
- inactive `DEBUG_SHADER_INCLUDES` code

Recommended next step:
- document shader program lifecycle ownership before routing any create,
  attach, detach, delete, or bind calls.

