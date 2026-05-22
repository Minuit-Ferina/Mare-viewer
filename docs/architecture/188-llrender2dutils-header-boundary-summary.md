# LLRender2DUtils Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llrender/llrender2dutils.h` so it no longer pulls shader and low-level GL headers into UI consumers by default.

## Source Changes

- `indra/llrender/llrender2dutils.h`
  - Removed the public `llglslshader.h` include.
  - Added forward declarations for `LLGLSLShader`, `LLRenderTarget`, and `LLTexture`.
  - Replaced the public `GLfloat` checkerboard alpha parameter with `F32`.
  - Added direct standard includes for the `std::list` and `std::string` types used by the header.
- `indra/llrender/llrender2dutils.cpp`
  - Added the implementation-local `llglslshader.h` include.
  - Replaced internal `GLfloat` scratch values with `F32` where no OpenGL ABI type is required.
- `indra/llrender/lluiimage.cpp`
  - Added explicit `llrender.h` and `v3math.h` includes for `gGL` and `LLVector3` usage that had been supplied transitively.
- `indra/llui/llfloater.cpp`
  - Added explicit `llgl.h` include for the existing direct `LLGLEnable` and `GL_CULL_FACE` usage that had been supplied transitively.

## Risk

Low-medium.

The only runtime-facing change is type spelling from `GLfloat` to `F32`, which is the same scalar representation for these UI helper paths. The practical risk is compile fallout from consumers that depended on `llrender2dutils.h` to provide shader or GL symbols indirectly. The first `llui` verification pass exposed this in `llfloater.cpp`; that dependency is now explicit.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llviewerwindow.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed after explicit `lluiimage.cpp` includes were added.
- `llui`: passed after explicit `llfloater.cpp` GL include was added.
- Targeted `llviewerwindow.cpp.o`: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

The next useful header-boundary candidates are larger and should be handled separately:

- `indra/llrender/llglslshader.h`
- `indra/llrender/llshadermgr.h`
- `indra/llrender/llgl.h`
