# LLGLSLShader UBO Binding Containment Summary

Branch: `phase3`
Source commit: `fb592eb91c`

## Scope Completed

Routed the `LLGLSLShader::mapUniforms()` UBO block binding call-through through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call family:
- `glUniformBlockBinding`

## Behavior Notes

`LLGLSLShader` still owns:
- UBO block name list
- block binding order
- `NUM_UNIFORM_BLOCKS` assertion
- `GL_INVALID_INDEX` guard
- bind/unbind ordering around uniform mapping

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
  change UBO policy or shader binding order.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 16
  to 15.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 97 to 98.

## Remaining LLGLSLShader Direct Calls

Still direct:
- program lifecycle and shader object ownership
- shader attach/detach/delete
- attribute binding before link
- program bind/unbind
- inactive `DEBUG_SHADER_INCLUDES` code

Recommended next step:
- document and decide whether attribute binding before link is a safe isolated
  containment packet; keep program bind/unbind and lifecycle separate.

