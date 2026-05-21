# LLGLSLShader Metadata Query Containment Summary

Branch: `phase3`
Source commit: `a68c711146`

## Scope Completed

Routed read-only `LLGLSLShader` metadata queries through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call families:
- `glGetError`
- `glGetUniformLocation`
- `glGetAttribLocation`
- `glGetProgramiv`
- `glGetActiveUniform`
- `glGetUniformBlockIndex`

## Behavior Notes

No cache ownership moved out of `LLGLSLShader`.

`LLGLSLShader` still owns:
- attribute and uniform cache population
- reserved uniform ordering policy
- texture channel assignment
- UBO binding decisions
- debug uniform validation

`llglcontainment.*` owns only the direct OpenGL read/query call-through.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

Runtime smoke:
- deferred, because this packet did not touch shader binding, uniform writes,
  draw order, or shader lifecycle.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 65
  to 51.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 74 to 78.

## Remaining LLGLSLShader Risk Areas

Still direct:
- program lifecycle and shader object ownership
- attach, detach, delete, create program
- attribute binding before link
- uniform block binding
- program bind/unbind
- uniform writes
- vertex attribute writes
- inactive `DEBUG_SHADER_INCLUDES` code

Recommended next step:
- keep lifecycle and binding contract-first; the next wrapper-only candidate is
  uniform setter containment, but it is broad and should be split by setter
  family.

