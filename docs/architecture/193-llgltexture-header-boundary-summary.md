# LLGLTexture Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llrender/llgltexture.h` so texture consumers no
longer receive `llgl.h` through the public texture wrapper header.

## Source Changes

- `indra/llrender/llgltexture.h`
  - Replaced the public `llgl.h` include with direct dependencies:
    `llgltypes.h`, `llpointer.h`, `llrender.h`, and `llunits.h`.
  - Kept the existing public texture API and scalar aliases unchanged.
  - Forward-declared `LLImageGL` and `LLImageRaw` explicitly.
- `indra/llrender/llgltexture.cpp`
  - Added the implementation-local `llimagegl.h` include because the
    implementation constructs and calls `LLImageGL` directly.

## Risk

Medium-low.

`llgltexture.h` is included from both UI and viewer texture paths, so compile
fallout risk is wider than the previous resource headers. Runtime behavior is
unchanged: the public methods still delegate to `LLImageGL`, and no call order
or GL state ownership changed.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llviewertexture.cpp.o newview/CMakeFiles/mare-viewer.dir/llworldmap.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewertexturelist.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted viewer texture `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

The remaining high-impact `llrender` public headers with direct `llgl.h`
exposure are:

- `indra/llrender/llrendertarget.h`
- `indra/llrender/llvertexbuffer.h`
- `indra/llrender/llglstates.h`
