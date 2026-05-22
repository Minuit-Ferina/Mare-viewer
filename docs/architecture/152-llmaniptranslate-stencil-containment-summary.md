# LLManipTranslate Stencil Containment Summary

Branch: `phase3`
Source commit: `ac9b1da44a`

## Scope Completed

Routed direct OpenGL cull/stencil calls in the disabled
`LLManipTranslate::highlightIntersection` path through `llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llmaniptranslate.cpp`

Added containment helpers:
- `setStencilFunction`
- `setStencilMask`
- `setStencilOperation`

Reused existing containment helper:
- `setCullFace`

Contained call families:
- `glCullFace`
- `glStencilFunc`
- `glStencilMask`
- `glStencilOp`

## Behavior Notes

`LLManipTranslate` still owns:
- the disabled `#if 0` state of the deprecated path
- grid cross-section ordering if the path is ever re-enabled
- selected stencil values
- cull face direction selection
- grid rendering

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llmaniptranslate.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview object build used a minimal PCH refresh because
  `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llmaniptranslate.cpp`: active direct `gl*` calls reduced from
  13 to 0.
- `indra/newview/llmaniptranslate.cpp`: raw `gl*` references reduced from 13
  to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 128 to 131.

Runtime smoke:
- deferred; the affected path remains disabled.
