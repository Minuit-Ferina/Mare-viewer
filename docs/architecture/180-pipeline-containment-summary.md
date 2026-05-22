# Pipeline Containment Summary

Branch: `phase3`
Source commit: `b1561fe3cf`

## Scope Completed

Closed the final direct runtime OpenGL callsites in:
- `indra/newview/pipeline.cpp`

## Changes

`pipeline.cpp`:
- routed query lifetime and transform-feedback query calls through
  `LLGLContainment`
- routed texture parameter calls through `LLGLContainment`
- routed clear color and clear mask calls through `LLGLContainment`
- routed polygon mode, polygon offset, line width, and point size calls through
  `LLGLContainment`
- routed framebuffer status checks through `LLGLContainment`
- routed viewport calls through `LLGLContainment`
- routed previous avatar palette matrix upload through `LLGLContainment`
- routed draw buffer selection through `LLGLContainment`
- removed remaining `gl*` raw refs from disabled/commented snippets

`llglcontainment.*`:
- added `getFramebufferStatus(target)` for the generic framebuffer status
  check used by `pipeline.cpp`

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `/opt/homebrew/bin/cmake -E touch /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_arm64.hxx /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_x86_64.hxx`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/pipeline.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview compile rebuilt the two PCH files and
  `pipeline.cpp.o`.
- No clean build was run.
- No full viewer link was run.

## Inventory Result

After regeneration:
- `indra/newview/pipeline.cpp`: active direct `gl*` calls reduced from 72 to
  0.
- `indra/newview/pipeline.cpp`: raw `gl*` refs reduced from 83 to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 159 to 160 because it now owns the framebuffer status wrapper.

Runtime smoke:
- deferred unless requested.
