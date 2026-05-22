# GLTFSceneManager Containment Summary

Branch: `phase3`
Source commit: `33c3a361c6`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/gltfscenemanager.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/gltfscenemanager.cpp`

Added containment helper:
- `bindBufferBase`

Reused existing containment helpers:
- `setActiveTexture`
- `bindTexture`
- `setTextureParameterInteger`
- `setPolygonMode`

Contained call families:
- `glBindBufferBase`
- `glActiveTexture`
- `glBindTexture`
- `glTexParameteri`
- `glPolygonMode`

## Behavior Notes

`GLTFSceneManager` still owns:
- GLTF batch iteration
- shader variant selection
- UBO binding slot selection
- texture channel selection
- sampler parameter values
- fallback texture choice
- debug raycast wireframe ordering

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/gltfscenemanager.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview object build used a minimal PCH refresh because
  `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/gltfscenemanager.cpp`: active direct `gl*` calls reduced from
  17 to 0.
- `indra/newview/gltfscenemanager.cpp`: raw `gl*` references reduced from 17
  to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 125 to 126.

Runtime smoke:
- deferred unless requested; a later check should render a GLTF object and
  optionally enable raycast debug overlay.
