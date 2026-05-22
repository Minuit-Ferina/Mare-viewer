# GLTF UBO Containment Summary

Branch: `phase3`
Source commit: `4b716e3f15`

## Scope Completed

Routed direct GLTF UBO buffer calls through existing `llglcontainment.*`
helpers.

Changed source files:
- `indra/newview/gltf/asset.cpp`
- `indra/newview/gltf/animation.cpp`

Reused existing containment helpers:
- `generateBufferObjects`
- `deleteBufferObjects`
- `bindBufferObject`
- `allocateBufferObjectStorage`

Contained call families:
- `glGenBuffers`
- `glDeleteBuffers`
- `glBindBuffer`
- `glBufferData`

## Behavior Notes

The GLTF owners still control:
- node UBO allocation timing
- material UBO allocation timing
- skin UBO lifetime
- matrix and material packing layouts
- upload sizes and usage flags

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/gltf/asset.cpp.o newview/CMakeFiles/mare-viewer.dir/gltf/animation.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- No `llglcontainment.h` change was required for this packet.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/gltf/asset.cpp`: active direct `gl*` calls reduced from 8 to
  0.
- `indra/newview/gltf/animation.cpp`: active direct `gl*` calls reduced from
  5 to 0.
- Both files now have 0 raw `gl*` references.
- `indra/llrender/llglcontainment.cpp`: unchanged at 132 active direct `gl*`
  calls.

Runtime smoke:
- deferred unless requested; later checks should include GLTF material and skin
  UBO paths.
