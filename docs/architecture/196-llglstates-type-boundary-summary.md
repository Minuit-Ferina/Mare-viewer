# LLGLStates Type Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llrender/llglstates.h` by replacing public raw
OpenGL scalar type spellings with project-owned `LLGL*` aliases.

`llglstates.h` remains an OpenGL state helper header and is still intentionally
included from `llgl.h`. This packet does not try to hide or replace the GL
state model.

## Source Changes

- `indra/llrender/llglstates.h`
  - Added an explicit `llgltypes.h` include.
  - Replaced `LLGLDepthTest` public/member/static `GLboolean` spellings with
    `LLGLboolean`.
  - Replaced `LLGLDepthTest` public/member/static `GLenum` spellings with
    `LLGLenum`.
- `indra/llrender/llgl.cpp`
  - Matched `LLGLDepthTest` static definitions and constructor signature to the
    updated alias spellings.

## Risk

Medium-low.

The underlying scalar representations are unchanged. `LLGLDepthTest` is widely
used in draw and manipulation paths, so targeted object compiles cover common
callers. The packet does not change depth enable, depth write, depth function,
or restoration behavior.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/pipeline.cpp.o newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o newview/CMakeFiles/mare-viewer.dir/llspatialpartition.cpp.o newview/CMakeFiles/mare-viewer.dir/llmaniptranslate.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewerdisplay.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted `LLGLDepthTest`-heavy `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

After this packet, the remaining raw GL header surface is concentrated in the
intentional low-level boundary headers:

- `indra/llrender/llgl.h`
- `indra/llrender/llglheaders.h`

New renderer-facing headers should prefer `llgltypes.h` aliases and keep
`llgl.h` local to implementation files unless the file is explicitly part of
the low-level OpenGL boundary.
