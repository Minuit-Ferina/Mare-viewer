# Occlusion Query Containment Summary

Branch: `phase3`
Source commit: `1cefb7ed6b`

## Scope Completed

Routed reflection-map and octree occlusion query OpenGL calls through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llreflectionmap.cpp`
- `indra/newview/llvieweroctree.cpp`

Added containment helper:
- `getQueryObjectUnsignedInteger`

Reused existing containment helpers:
- `generateQueries`
- `deleteQueries`
- `beginQuery`
- `endQuery`

Contained call families:
- `glGenQueries`
- `glDeleteQueries`
- `glBeginQuery`
- `glEndQuery`
- `glGetQueryObjectuiv`

## Behavior Notes

The original owners still control:
- query object allocation and pooling
- query mode selection
- availability and result interpretation
- occlusion timeout behavior
- pending query tracking
- occlusion state mutation
- reflection probe occlusion policy

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llreflectionmap.cpp.o newview/CMakeFiles/mare-viewer.dir/llvieweroctree.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview object build used a minimal PCH refresh because
  `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llreflectionmap.cpp`: active direct `gl*` calls reduced from
  6 to 0.
- `indra/newview/llvieweroctree.cpp`: active direct `gl*` calls reduced from
  5 to 0.
- Both files now have 0 raw `gl*` references.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 127 to 128.

Runtime smoke:
- deferred unless requested; later checks should cover occlusion-enabled scenes
  and reflection probes.
