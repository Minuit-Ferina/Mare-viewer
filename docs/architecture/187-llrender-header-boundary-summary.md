# LLRender Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows the public `LLRender` header boundary:

- `indra/llrender/llrender.h` no longer includes `llglheaders.h`.
- Public `LLRender` and `LLTexUnit` signatures that used OpenGL scalar aliases now use project-owned scalar aliases from `llgltypes.h`.
- `indra/llrender/llrender.cpp` includes `llglheaders.h` directly because it still owns low-level OpenGL constants and ABI-facing implementation details.
- `indra/llrender/llimagegl.h` no longer relies on transitive `GLuint` exposure from `llrender.h` for `setTexName(...)`.

## Source Changes

- `indra/llrender/llrender.h`
  - Replaced the public `llglheaders.h` include with `llgltypes.h`.
  - Replaced public `GLint`, `GLuint`, `GLfloat`, and `GLubyte` spelling with `LLGLint`, `LLGLuint`, `LLGLfloat`, and `U8`.
- `indra/llrender/llrender.cpp`
  - Added the implementation-local `llglheaders.h` include.
  - Matched the updated public type spellings in method definitions.
- `indra/llrender/llimagegl.h`
  - Changed `setTexName(GLuint)` to `setTexName(LLGLuint)`.

## Risk

Medium-low.

The runtime behavior should remain unchanged because the replacement aliases map to the same underlying scalar types used by the existing OpenGL aliases on the supported build. The main risk is compile-surface fallout from files that were relying on `llrender.h` to provide OpenGL typedefs transitively.

This risk was partially exercised by `llrender/fast`, `llui`, and targeted `newview` object compiles.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolmaterials.cpp.o newview/CMakeFiles/mare-viewer.dir/llspatialpartition.cpp.o newview/CMakeFiles/mare-viewer.dir/llface.cpp.o newview/CMakeFiles/mare-viewer.dir/pipeline.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

Remaining header-level OpenGL exposure is not fully removed. The next small packets should inspect public headers that still expose GL ABI spellings, especially:

- `indra/llrender/llgl.h`
- `indra/llrender/llglslshader.h`
- `indra/llrender/llshadermgr.h`
- `indra/llrender/llrender2dutils.h`
- `indra/llrender/llvertexbuffer.h`
- `indra/newview/marefsr2upscaler.h`
