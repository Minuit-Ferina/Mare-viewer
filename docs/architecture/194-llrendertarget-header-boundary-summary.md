# LLRenderTarget Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llrender/llrendertarget.h` so render-target
consumers no longer receive `llgl.h` through the public render-target header.

## Source Changes

- `indra/llrender/llrendertarget.h`
  - Replaced the public `llgl.h` include with `llgltypes.h`.
  - Kept `llrender.h` because the public API exposes `LLTexUnit` enum types.
  - Added an explicit `<vector>` include for render-target attachment storage.
- `indra/llrender/llrendertarget.cpp`
  - No behavior change. The implementation already keeps `llgl.h` local for
    GL debug state and framebuffer implementation details.

## Risk

Medium.

`LLRenderTarget` is central to the pipeline, reflection probes, dynamic
textures, and scene monitor code. This packet is intentionally limited to the
public include boundary: no FBO binding, attachment, viewport, clear, flush, or
stack behavior changed.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/pipeline.cpp.o newview/CMakeFiles/mare-viewer.dir/llreflectionmapmanager.cpp.o newview/CMakeFiles/mare-viewer.dir/llheroprobemanager.cpp.o newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o newview/CMakeFiles/mare-viewer.dir/llscenemonitor.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted render-target-heavy `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

The remaining `llrender` public headers with direct `llgl.h` exposure are now:

- `indra/llrender/llvertexbuffer.h`
- `indra/llrender/llglstates.h`
