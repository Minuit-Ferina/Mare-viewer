# LLGLSLShader Debug Include Containment Summary

Branch: `phase3`
Source commit: `5cac3e8889`

## Scope Completed

Routed the disabled `DEBUG_SHADER_INCLUDES` OpenGL calls through existing
`llglcontainment.*` helpers.

Changed source file:
- `indra/llrender/llglslshader.cpp`

Contained call families:
- `glGetShaderiv`
- `glGetProgramInfoLog`

Also changed:
- Reworded a nearby comment so generated inventory no longer records a false
  positive `gl*` reference.

## Behavior Notes

`DEBUG_SHADER_INCLUDES` remains disabled.

`LLGLSLShader` still owns:
- the debug block
- debug output formatting
- info log buffer lifetime

`llglcontainment.*` already owns the call-through wrappers used by this block.

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
  from 3 to 0.

`LLGLSLShader` no longer has direct `gl*` references in generated inventory.

