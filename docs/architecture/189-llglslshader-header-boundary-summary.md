# LLGLSLShader Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llrender/llglslshader.h` so consumers of the shader wrapper no longer receive `llgl.h` and the platform OpenGL headers through this public header.

## Source Changes

- `indra/llrender/llglslshader.h`
  - Replaced the public `llgl.h` include with `llgltypes.h`.
  - Added direct STL includes for containers used by the header.
  - Replaced public OpenGL scalar spellings with `LLGL*` aliases:
    `GLuint`, `GLint`, `GLfloat`, `GLboolean`, and `GLenum`.
- `indra/llrender/llglslshader.cpp`
  - Added the implementation-local `llgl.h` include.
  - Matched method definitions and internal shader wrapper bookkeeping to the updated `LLGL*` spellings where no raw OpenGL ABI type is required in the public contract.

## Risk

Medium.

`LLGLSLShader` is widely included by render, UI, and viewer code. The runtime behavior should remain unchanged because the `LLGL*` aliases map to the same scalar types, but this packet intentionally removes a broad transitive include path. The main risk is compile fallout in consumers that relied on `llglslshader.h` for unrelated `llgl.h` symbols.

The first verification pass did not expose additional consumers needing explicit `llgl.h`.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolmaterials.cpp.o newview/CMakeFiles/mare-viewer.dir/llreflectionmapmanager.cpp.o newview/CMakeFiles/mare-viewer.dir/llheroprobemanager.cpp.o newview/CMakeFiles/mare-viewer.dir/rlveffects.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted `newview` shader-heavy object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

Remaining public headers that still expose low-level GL types or include `llgl.h` should be handled separately, especially:

- `indra/llrender/llshadermgr.h`
- `indra/llrender/llgl.h`
- `indra/llrender/llglstates.h`
