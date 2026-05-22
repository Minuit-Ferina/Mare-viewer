# LLAppViewer Containment Summary

Branch: `phase3`
Source commit: `681b56d5b9`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llappviewer.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llappviewer.cpp`

Added containment helper:
- `getString`

Reused existing containment helper:
- `deleteTextures`

Contained call families:
- `glGetString`
- `glDeleteTextures`

## Behavior Notes

`LLAppViewer` still owns:
- viewer information collection
- GPU vendor, renderer, and OpenGL version field population
- deliberate driver crash behavior

The driver crash path still passes the existing null texture pointer through to
the OpenGL call-through.

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `/opt/homebrew/bin/cmake -E touch /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_arm64.hxx /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_x86_64.hxx`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llappviewer.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview object build used a minimal PCH refresh because
  `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llappviewer.cpp`: active direct `gl*` calls reduced from 4 to
  0.
- `indra/newview/llappviewer.cpp`: raw `gl*` references reduced from 4 to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 135 to 136.

Runtime smoke:
- deferred unless requested; later checks should verify viewer info still
  reports GPU vendor, renderer, and OpenGL version.
