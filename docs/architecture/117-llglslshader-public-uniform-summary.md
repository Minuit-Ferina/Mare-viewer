# LLGLSLShader Public Uniform Containment Summary

Branch: `phase3`
Source commit: `e2a6008f46`

## Scope Completed

Routed public `LLGLSLShader::uniform*` setter call-throughs through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call families:
- scalar integer uniforms
- scalar float uniforms
- integer vector uniforms
- unsigned integer vector uniforms
- float vector uniforms
- matrix uniforms

## Behavior Notes

No caching or policy moved out of `LLGLSLShader`.

`LLGLSLShader` still owns:
- bound-shader assertions
- uniform bounds checks
- cached value comparisons
- `mValue` updates
- stop-gl-error ordering around matrix writes

`llglcontainment.*` owns only the direct OpenGL uniform write call-through and
count/transpose conversions.

The existing `uniform4iv(U32 index, ...)` behavior was preserved exactly: it
continues to call the one-component integer vector API through containment.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

Runtime smoke:
- deferred, because this packet moved direct setter call-throughs without
  changing cache decisions, binding order, or shader lifecycle.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 49
  to 18.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 80 to 95.

## Remaining LLGLSLShader Direct Calls

Still direct:
- program lifecycle calls
- shader attach/detach/delete calls
- attribute binding before link
- uniform block binding
- program bind/unbind
- vertex attribute writes
- inactive `DEBUG_SHADER_INCLUDES` code

Recommended next step:
- route the two vertex attribute setter calls next; keep program lifecycle and
  program binding contract-first.

