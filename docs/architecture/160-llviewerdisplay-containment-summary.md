# LLViewerDisplay Containment Summary

Branch: `phase3`
Source commit: `664430cbc7`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llviewerdisplay.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/newview/llviewerdisplay.cpp`

Reused existing containment helpers:
- `clearBuffers`
- `setClearColor`
- `setViewport`
- `setPolygonMode`

Contained call families:
- `glClear`
- `glClearColor`
- `glViewport`
- `glPolygonMode`

Also removed raw `glColorMask` references from a disabled/commented floater
occlusion block by rewriting the comments without changing code.

## Behavior Notes

`LLViewerDisplay` still owns:
- startup display sequencing
- resize-frame skip behavior
- dynamic texture depth clear timing
- shadow/impostor viewport setup timing
- wireframe and cube snapshot clear color choices
- UI/HUD render ordering
- dirty UI buffer redraw behavior

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llviewerdisplay.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- No `llrender` helper changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llviewerdisplay.cpp`: active direct `gl*` calls reduced from
  19 to 0.
- `indra/newview/llviewerdisplay.cpp`: raw `gl*` references reduced from 19 to
  0.

Runtime smoke:
- deferred unless requested; later checks should cover login, scene render,
  resize, and UI buffer redraw.
