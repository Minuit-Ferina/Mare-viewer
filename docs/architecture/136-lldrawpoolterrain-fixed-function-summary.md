# LLDrawPoolTerrain Fixed Function Containment Summary

Branch: `phase3`
Source commit: `487ae3a0f0`

## Scope Completed

Routed fixed-function terrain OpenGL calls in
`indra/newview/lldrawpoolterrain.cpp` through `llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/newview/lldrawpoolterrain.cpp`

Contained call families:
- `glPolygonOffset`
- `glEnable`
- `glDisable`
- `glTexGeni`
- `glTexGenfv`

## Behavior Notes

`LLDrawPoolTerrain` still owns:
- terrain render pass order
- texture unit activation order
- texture bindings
- texture matrix setup
- generated object-plane values
- blend state
- shader selection

`llglcontainment.*` owns only the direct fixed-function OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/lldrawpoolterrain.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.
- `lldrawpoolterrain.cpp.o` compiled successfully.

## Inventory Result

After regeneration:
- `indra/newview/lldrawpoolterrain.cpp`: active direct `gl*` calls reduced
  from 61 to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 117 to 121.

Runtime smoke:
- recommended at a later checkpoint because this path affects legacy terrain
  detail passes.

