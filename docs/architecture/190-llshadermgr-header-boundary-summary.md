# LLShaderMgr Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llrender/llshadermgr.h` so shader-manager consumers no longer receive `llgl.h` through the public header.

## Source Changes

- `indra/llrender/llshadermgr.h`
  - Replaced the public `llgl.h` include with `llgltypes.h`.
  - Added direct STL includes used by the header.
  - Replaced public OpenGL scalar spellings with project-owned scalar aliases:
    `GLuint` to `LLGLuint`, `GLenum` to `LLGLenum`, `GLsizei` to `S32`, and `GLchar` to `char`.
- `indra/llrender/llshadermgr.cpp`
  - Added the implementation-local `llgl.h` include.
  - Matched shader-manager definitions and local bookkeeping to the updated type spellings where no raw OpenGL ABI type is required in the public contract.

## Risk

Medium-low.

The exposed types remain the same underlying scalar representations. The main risk is compile fallout from callers that depended on `llshadermgr.h` to provide unrelated GL declarations. The verification targets include `LLViewerShaderMgr` and shader-heavy draw/lighting paths.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llviewershadermgr.cpp.o newview/CMakeFiles/mare-viewer.dir/lldrawpoolmaterials.cpp.o newview/CMakeFiles/mare-viewer.dir/llreflectionmapmanager.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted `newview` shader manager and shader-heavy object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

The next header-boundary candidates are lower-level and should be treated as explicit GL boundary work:

- `indra/llrender/llgl.h`
- `indra/llrender/llglstates.h`
