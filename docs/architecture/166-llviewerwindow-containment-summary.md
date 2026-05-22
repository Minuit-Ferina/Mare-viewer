# LLViewerWindow Containment Summary

Branch: `phase3`
Source commit: `16a5425fdc`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llviewerwindow.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llviewerwindow.cpp`

Added containment helper:
- `readPixels`

Reused existing containment helpers:
- `setCullFace`
- `clearBuffers`
- `setViewport`

Contained call families:
- `glReadPixels`
- `glCullFace`
- `glClear`
- `glViewport`

Also removed raw `glReadPixels` and `glViewport` references from comments by
rewriting them without changing code.

## Behavior Notes

`LLViewerWindow` still owns:
- debug pixel color readback decisions
- selected light debug draw order
- snapshot orchestration
- raw snapshot color/depth readback dimensions and offsets
- simple snapshot readback
- cube snapshot setup
- 2D and 3D viewport rectangle setup

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `/opt/homebrew/bin/cmake -E touch /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_arm64.hxx /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_x86_64.hxx`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llviewerwindow.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview object build used a minimal PCH refresh because
  `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llviewerwindow.cpp`: active direct `gl*` calls reduced from
  12 to 0.
- `indra/newview/llviewerwindow.cpp`: raw `gl*` references reduced from 14 to
  0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 136 to 137.

Runtime smoke:
- deferred unless requested; later checks should include screenshot capture,
  resize, and basic scene render.
