# LLReflectionMapManager Containment Summary

Branch: `phase3`
Source commit: `71a9fca7fb`

## Scope Completed

Routed direct OpenGL calls in `indra/newview/llreflectionmapmanager.cpp`
through `llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/llreflectionmapmanager.cpp`

Added containment helper:
- `copyTextureSubImage3D`

Reused existing containment helpers:
- `setTextureImage2D`
- `generateTextureMipmap`
- `setViewport`
- `generateBufferObjects`
- `bindBufferObject`
- `allocateBufferObjectStorage`
- `bindBufferBase`
- `deleteBufferObjects`

Contained call families:
- `glTexImage2D`
- `glGenerateMipmap`
- `glCopyTexSubImage3D`
- `glViewport`
- `glGenBuffers`
- `glBindBuffer`
- `glBufferData`
- `glBindBufferBase`
- `glDeleteBuffers`

## Behavior Notes

`LLReflectionMapManager` still owns:
- EXR preview texture setup
- reflection probe update ordering
- cube face and mip selection
- radiance and irradiance viewport sizing
- reflection probe UBO data layout and update timing
- render target binding and flushing

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/llreflectionmapmanager.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The targeted newview object build used a minimal PCH refresh because
  `llglcontainment.h` changed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/llreflectionmapmanager.cpp`: active direct `gl*` calls
  reduced from 15 to 0.
- `indra/newview/llreflectionmapmanager.cpp`: raw `gl*` references reduced
  from 15 to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 126 to 127.

Runtime smoke:
- deferred unless requested; a later check should use a scene with reflection
  probes enabled.
