# LLGLSLShader Program Binding Containment Summary

Branch: `phase3`
Source commit: `5c86381bdb`

## Scope Completed

Routed `LLGLSLShader` program bind/unbind call-throughs through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call family:
- `glUseProgram`

## Behavior Notes

`LLGLSLShader` still owns:
- redundant bind avoidance
- `gGL.flush()` ordering
- `LLVertexBuffer::unbind()` ordering
- pending profile query reads
- `sCurBoundShader` and `sCurBoundShaderPtr`
- `placeProfileQuery()`
- dirty uniform updates

`llglcontainment.*` owns only the direct OpenGL program bind call-through.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

Runtime smoke:
- recommended at the next practical checkpoint because program binding is
  global OpenGL state, even though this packet only moved call-through wrappers.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 14
  to 12.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 99 to 100.

## Remaining LLGLSLShader Direct Calls

Still direct:
- program lifecycle and shader object ownership
- shader attach/detach/delete
- inactive `DEBUG_SHADER_INCLUDES` code

Recommended next step:
- stop wrapper-only routing here and document shader program lifecycle/attach
  ownership before touching the remaining active calls.

