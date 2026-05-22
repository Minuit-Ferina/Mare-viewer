# Debug Overlay Fixed-Function Containment Summary

Branch: `phase3`
Source commit: `f18c5c5f4d`

## Scope Completed

Routed debug-overlay fixed-function calls through existing
`llglcontainment.*` helpers.

Changed source files:
- `indra/newview/llglsandbox.cpp`
- `indra/newview/llselectmgr.cpp`

Reused existing containment helpers:
- `setLineWidth`
- `setPolygonMode`

Contained call families:
- `glLineWidth`
- `glPolygonMode`

## Behavior Notes

The original owners still control:
- debug beacon geometry and selected line widths
- sun/moon beacon geometry and selected line width
- selected-object wireframe order
- selection shader rebinding
- hidden-selection behavior

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llglsandbox.cpp.o newview/CMakeFiles/mare-viewer.dir/llselectmgr.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- No `llglcontainment.h` change was required for this packet.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llglsandbox.cpp`: active direct `gl*` calls reduced from 5
  to 0.
- `indra/newview/llselectmgr.cpp`: active direct `gl*` calls reduced from 5
  to 0.
- `indra/llrender/llglcontainment.cpp`: unchanged at 127 active direct `gl*`
  calls.

Runtime smoke:
- deferred unless requested; later checks should cover debug beacons and object
  selection wireframe.
