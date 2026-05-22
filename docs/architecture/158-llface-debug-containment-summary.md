# LLFace Debug Containment Summary

Branch: `phase3`
Source commit: `a8d7d008ab`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llface.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llface.cpp`

Added containment helpers:
- `enableClientState`
- `disableClientState`
- `setTextureCoordinatePointer`

Reused existing containment helpers:
- `setPolygonOffset`
- `setVertexPointer`
- `drawElements`
- `setLineWidth`
- `setPolygonMode`

Contained call families:
- `glPolygonOffset`
- `glVertexPointer`
- `glEnableClientState`
- `glTexCoordPointer`
- `glDrawElements`
- `glDisableClientState`
- `glLineWidth`
- `glPolygonMode`

## Behavior Notes

`LLFace` still owns:
- selected-face render ordering
- active object versus region transform selection
- selected-face colors and blend state
- disabled rigged face debug draw block
- vertex and index data source choices

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llface.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview object build used a minimal PCH refresh because
  `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llface.cpp`: active direct `gl*` calls reduced from 9 to 0.
- `indra/newview/llface.cpp`: raw `gl*` references reduced from 9 to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 132 to 135.

Runtime smoke:
- deferred unless requested; later checks should select/edit a face and verify
  the wireframe overlay.
