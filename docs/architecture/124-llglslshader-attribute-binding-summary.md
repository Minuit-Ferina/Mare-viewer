# LLGLSLShader Attribute Binding Containment Summary

Branch: `phase3`
Source commit: `2950dcc91e`

## Scope Completed

Routed the reserved attribute binding call in `LLGLSLShader::mapAttributes()`
through `llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call family:
- `glBindAttribLocation`

## Behavior Notes

`LLGLSLShader` still owns:
- the `mUsingBinaryProgram` branch
- reserved attribute iteration
- attribute index/name mapping
- link timing after the binding loop
- post-link attribute cache population

`llglcontainment.*` owns only the direct OpenGL call-through.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

Runtime smoke:
- deferred, because this packet moved one call-through wrapper and did not
  change attribute ordering or link timing.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 15
  to 14.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 98 to 99.

## Remaining LLGLSLShader Direct Calls

Still direct:
- program lifecycle and shader object ownership
- shader attach/detach/delete
- program bind/unbind
- inactive `DEBUG_SHADER_INCLUDES` code

Recommended next step:
- keep program bind/unbind and lifecycle as separate contract-first packets.

