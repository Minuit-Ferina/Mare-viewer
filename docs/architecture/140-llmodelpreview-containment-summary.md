# LLModelPreview Debug Containment Summary

Branch: `phase3`
Source commit: `427c70872e`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llmodelpreview.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llmodelpreview.cpp`

Added containment helper:
- `setPointSize`

Reused existing containment helpers:
- `setLineWidth`
- `setPolygonMode`
- `clearBuffers`

Contained call families:
- `glLineWidth`
- `glPolygonMode`
- `glPointSize`
- `glClear`

## Behavior Notes

`LLModelPreview` still owns:
- preview mode selection
- mesh edge overlay ordering
- physics overlay ordering
- degenerate triangle and point overlay ordering
- selected line and point widths
- depth clear placement before physics preview rendering
- vertex buffer draw calls

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The direct Makefile object target again needed a minimal `newview` PCH
  refresh after `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llmodelpreview.cpp`: active direct `gl*` calls reduced from
  17 to 0.
- `indra/newview/llmodelpreview.cpp`: raw `gl*` references reduced from 17 to
  0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 124 to 125.

Runtime smoke:
- deferred unless requested; a later check should open the model upload/preview
  UI and toggle mesh edge, physics, and degenerate overlays.
