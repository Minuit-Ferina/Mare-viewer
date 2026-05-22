# LLSpatialPartition Debug Containment Summary

Branch: `phase3`
Source commit: `4333a6dccf`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llspatialpartition.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llspatialpartition.cpp`

Added containment helpers:
- `setPolygonMode`
- `setVertexPointer`
- `drawElements`

Reused existing containment helpers:
- `setLineWidth`
- `setPolygonOffset`

Contained call families:
- `glLineWidth`
- `glPolygonMode`
- `glPolygonOffset`
- `glVertexPointer`
- `glDrawElements`

## Behavior Notes

`LLSpatialPartition` still owns:
- spatial traversal order
- debug overlay branch selection
- wireframe/fill transition order
- physics hull vertex and index data
- matrix synchronization before physics hull drawing
- debug colors and line widths

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llspatialpartition.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The direct Makefile object target used a stale `newview` PCH after
  `llglcontainment.h` changed.
- Touching only the generated PCH include files in the build tree forced a PCH
  rebuild without a clean build.

## Inventory Result

After regeneration:
- `indra/newview/llspatialpartition.cpp`: active direct `gl*` calls reduced
  from 27 to 0.
- `indra/newview/llspatialpartition.cpp`: raw `gl*` references reduced from
  27 to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 121 to 124.

Runtime smoke:
- deferred unless requested; this packet mainly affects debug overlays and
  physics hull debug rendering.
