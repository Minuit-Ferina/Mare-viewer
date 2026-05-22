# FSR2 Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/newview/marefsr2upscaler.h` so FSR2 header
consumers no longer receive `llgl.h`.

The FSR2 backend remains a raw OpenGL compute implementation. This packet only
changes type/include ownership at the public header boundary.

## Source Changes

- `indra/newview/marefsr2upscaler.h`
  - Replaced the public `llgl.h` include with `llgltypes.h`.
  - Replaced public/private raw `GLuint` spellings with `LLGLuint`.
  - Replaced the texture helper raw `GLenum` parameter with `LLGLenum`.
- `indra/newview/marefsr2upscaler.cpp`
  - Added the implementation-local `llgl.h` include.
  - Matched FSR2 member helper definitions to the updated alias spellings.

## Risk

Low for Darwin local builds; medium for FSR2-enabled builds.

`MARE_ENABLE_FSR2` is disabled in the local Darwin build path, so this packet
only validates header consumers on this machine. The underlying scalar
representations are unchanged, but the full FSR2 implementation should be
compiled on a configuration where `MARE_ENABLE_FSR2` is active before treating
this as fully validated.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/maretaaupscaler.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewercamera.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- Targeted Darwin header consumers: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

Not run:

- `marefsr2upscaler.cpp.o`, because `marefsr2upscaler.cpp` is excluded when
  `MARE_ENABLE_FSR2` is disabled on this Darwin build.

## Follow-Up

Validate `marefsr2upscaler.cpp` on a non-Darwin or otherwise FSR2-enabled
configuration before making further FSR2 implementation changes.
