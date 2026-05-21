# LLGLSLShader Vertex Attribute Containment Summary

Branch: `phase3`
Source commit: `46a9dc0c79`

## Scope Completed

Routed the two `LLGLSLShader` vertex attribute setter call-throughs through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call families:
- `glVertexAttrib4f`
- `glVertexAttrib4fv`

## Behavior Notes

`LLGLSLShader` still owns:
- the `mAttribute[index] > 0` guard
- attribute index lookup and storage

`llglcontainment.*` owns only the direct vertex attribute write call-through.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

Runtime smoke:
- deferred, because this packet only moved two call-through wrappers.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 18
  to 16.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 95 to 97.

## Remaining LLGLSLShader Direct Calls

Remaining direct calls are no longer simple setter wrappers:
- program lifecycle and shader object ownership
- shader attach/detach/delete
- attribute binding before link
- uniform block binding
- program bind/unbind
- inactive `DEBUG_SHADER_INCLUDES` code

Recommended next step:
- document the remaining lifecycle/binding calls before routing any of them.

