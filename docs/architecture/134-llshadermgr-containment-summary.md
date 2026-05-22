# LLShaderMgr Containment Summary

Branch: `phase3`
Source commit: `c7d829976c`

## Scope Completed

Routed active direct OpenGL calls in `indra/llrender/llshadermgr.cpp` through
`llglcontainment.*`.

Changed source files:
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llshadermgr.cpp`

Contained call families:
- shader/program log queries
- shader/program status queries
- shader creation and deletion
- shader source upload and compile
- program link and validate
- program binary cache load/save
- GL error reads

## Behavior Notes

`LLShaderMgr` still owns:
- source file loading
- generated shader source text
- compile/link retry policy
- shader cache lookup and persistence
- shader feature selection
- logging and warning branches

`llglcontainment.*` owns only the OpenGL call-through wrappers and GL boundary
type conversion.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `python3 tools/architecture/source_inventory.py .`

Build result:
- `libllrender.a` relinked successfully.

## Inventory Result

After regeneration:
- `indra/llrender/llshadermgr.cpp`: active direct `gl*` calls reduced from 32
  to 0.
- `indra/llrender/llglcontainment.cpp`: active direct `gl*` calls increased
  from 107 to 117.

Runtime smoke:
- recommended at the next checkpoint because shader compile/link startup paths
  now route through containment wrappers.

