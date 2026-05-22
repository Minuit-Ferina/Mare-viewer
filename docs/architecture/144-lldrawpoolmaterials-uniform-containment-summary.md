# LLDrawPoolMaterials Uniform Containment Summary

Branch: `phase3`
Source commit: `b8e6818263`

## Scope Completed

Routed direct shader uniform OpenGL calls in
`indra/newview/lldrawpoolmaterials.cpp` through existing
`llglcontainment.*` helpers.

Changed source file:
- `indra/newview/lldrawpoolmaterials.cpp`

Reused existing containment helpers:
- `setUniformFloat`
- `setUniformFloatVector4`

Contained call families:
- `glUniform1f`
- `glUniform4fv`

## Behavior Notes

`LLDrawPoolMaterials` still owns:
- deferred material pass selection
- uniform location lookup
- cached uniform value comparisons
- texture channel binding
- draw batch iteration
- matrix palette upload

`llglcontainment.*` owns only the direct OpenGL uniform call-throughs.

## Verification

Completed:
- `git diff --check`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/lldrawpoolmaterials.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- No `llglcontainment.h` change was required for this packet.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/lldrawpoolmaterials.cpp`: active direct `gl*` calls reduced
  from 8 to 0.
- `indra/newview/lldrawpoolmaterials.cpp`: raw `gl*` references reduced from
  8 to 0.
- `indra/llrender/llglcontainment.cpp`: unchanged at 126 active direct `gl*`
  calls.

Runtime smoke:
- deferred unless requested; a later check should include deferred material
  surfaces.
