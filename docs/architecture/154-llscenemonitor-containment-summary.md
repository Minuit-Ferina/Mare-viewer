# LLSceneMonitor Containment Summary

Branch: `phase3`
Source commit: `b98fa4badb`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llscenemonitor.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llscenemonitor.cpp`

Added containment helper:
- `bindFramebuffer`

Reused existing containment helpers:
- `copyTextureSubImage2D`
- `setColorMask`
- `beginQuery`
- `endQuery`
- `getQueryObjectUnsignedInteger`

Contained call families:
- `glBindFramebuffer`
- `glCopyTexSubImage2D`
- `glColorMask`
- `glBeginQuery`
- `glEndQuery`
- `glGetQueryObjectuiv`

## Behavior Notes

`LLSceneMonitor` still owns:
- capture cadence
- framebuffer restore target
- diff target sizing
- color-mask conditions
- query lifecycle and result interpretation
- Windows-only query guarded path

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llscenemonitor.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview object build used a minimal PCH refresh because
  `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llscenemonitor.cpp`: active direct `gl*` calls reduced from
  11 to 0.
- `indra/newview/llscenemonitor.cpp`: raw `gl*` references reduced from 11 to
  0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 131 to 132.

Runtime smoke:
- deferred unless requested; later checks should enable scene loading monitor.
