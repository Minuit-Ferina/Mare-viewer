# Cube Map Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llrender/llcubemap.h` and
`indra/llrender/llcubemaparray.h` so cube-map consumers no longer receive
`llgl.h` through those public headers.

## Source Changes

- `indra/llrender/llcubemap.h`
  - Replaced the public `llgl.h` include with explicit type and ownership
    includes.
  - Kept `LLImageGL` as an explicit header dependency because
    `getResolution()` calls `LLImageGL::getWidth(...)` inline.
  - Replaced public `GLuint` spelling with `LLGLuint`.
- `indra/llrender/llcubemap.cpp`
  - Added the implementation-local `llgl.h` include.
  - Matched `getGLName()` to the updated public return type.
- `indra/llrender/llcubemaparray.h`
  - Replaced the public `llgl.h` include with `llgltypes.h`,
    `llpointer.h`, and `llrefcount.h`.
  - Replaced public `GLenum` and `GLuint` spellings with `LLGLenum` and
    `LLGLuint`.
- `indra/llrender/llcubemaparray.cpp`
  - Added the implementation-local `llgl.h` include.
  - Matched static target storage and `getGLName()` to the updated public
    type spellings.

## Risk

Low.

The exposed scalar types keep the same underlying representation. The main
risk is compile fallout from consumers that relied on the cube-map headers to
provide unrelated GL declarations. The packet keeps real OpenGL declarations
local to the implementation files that already use GL state and constants.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llreflectionmapmanager.cpp.o newview/CMakeFiles/mare-viewer.dir/llheroprobemanager.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted reflection-probe `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

The next low-risk header-boundary candidates are:

- `indra/llrender/llpostprocess.h`
- `indra/llrender/llrendersphere.h`

The heavier remaining headers still need separate packets:

- `indra/llrender/llgltexture.h`
- `indra/llrender/llrendertarget.h`
- `indra/llrender/llvertexbuffer.h`
- `indra/llrender/llglstates.h`
