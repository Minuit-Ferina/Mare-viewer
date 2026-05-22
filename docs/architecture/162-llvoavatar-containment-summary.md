# LLVOAvatar Containment Summary

Branch: `phase3`
Source commit: `90a7facd43`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llvoavatar.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/newview/llvoavatar.cpp`

Reused existing containment helpers:
- `setLineWidth`
- `generateQueries`
- `beginQuery`
- `endQuery`
- `getQueryObjectUnsignedInteger64`

Contained call families:
- `glLineWidth`
- `glGenQueries`
- `glBeginQuery`
- `glEndQuery`
- `glGetQueryObjectui64v`

## Behavior Notes

`LLVOAvatar` still owns:
- impostor debug outline rendering decisions
- GPU profile query allocation timing
- GPU profile pending state
- retry scheduling
- render-time conversion and debug text update

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llvoavatar.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- No `llrender` helper changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llvoavatar.cpp`: active direct `gl*` calls reduced from 6 to
  0.
- `indra/newview/llvoavatar.cpp`: raw `gl*` references reduced from 6 to 0.

Runtime smoke:
- deferred unless requested; later checks should include an avatar-heavy scene
  and impostor debug/profile UI if those modes are used.
