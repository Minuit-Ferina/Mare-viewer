# LLGLSLShader Texture Channel Uniform Containment Summary

Branch: `phase3`
Source commit: `549c25f02a`

## Scope Completed

Routed only `LLGLSLShader::mapUniformTextureChannel(...)` sampler uniform writes
through `llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llglslshader.cpp`

Contained call families:
- `glUniform1i`
- `glUniform1iv`

## Behavior Notes

`LLGLSLShader` still owns:
- sampler uniform detection
- texture channel assignment
- sampler array channel allocation
- `mActiveTextureChannels`

`llglcontainment.*` owns only the direct uniform write call-through.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

Runtime smoke:
- deferred, because this packet only moved two sampler uniform call-throughs.

## Inventory Result

After regeneration:
- `indra/llrender/llglslshader.cpp`: active direct `gl*` calls reduced from 51
  to 49.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 78 to 80.

## Remaining LLGLSLShader Uniform Work

Still direct:
- public scalar uniform setters
- public vector uniform setters
- public matrix uniform setters
- unsigned integer vector uniform setters
- uniform block binding

Recommended next step:
- route public `LLGLSLShader::uniform*` setters by scalar/vector/matrix family.

