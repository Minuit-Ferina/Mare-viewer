# LLVertexBuffer Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llrender/llvertexbuffer.h` so vertex-buffer
consumers no longer receive `llgl.h` through the public vertex-buffer header.

## Source Changes

- `indra/llrender/llvertexbuffer.h`
  - Replaced the public `llgl.h` include with `llgltypes.h` and
    `llrefcount.h`.
  - Added an explicit `LLWindow` forward declaration for the existing
    `initClass(...)` signature.
  - Replaced the private `flush_vbo(...)` raw `GLenum` parameter spelling with
    `LLGLenum`.
  - Moved the default index-type GL constant out of the header by initializing
    `mIndicesType` in the constructor.
- `indra/llrender/llvertexbuffer.cpp`
  - Added the implementation-local `llgl.h` include for GL manager,
    diagnostics, and `STOP_GLERROR` usage.
  - Initialized `mIndicesType` with `GL_UNSIGNED_SHORT` in the constructor.
  - Matched `flush_vbo(...)` to the updated alias type.
- `indra/llrender/llrender.cpp`
  - Added the implementation-local `llgl.h` include because it previously
    received `gGLManager` and `stop_glerror` transitively through
    `llvertexbuffer.h`.

## Risk

Medium.

`LLVertexBuffer` is a central draw-path type. This packet keeps behavior
unchanged by preserving the underlying GL enum values and moving only include
and type-spelling ownership. The main risk is compile fallout from callers that
depended on `llvertexbuffer.h` to provide unrelated GL diagnostics or manager
declarations.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolavatar.cpp.o newview/CMakeFiles/mare-viewer.dir/lldrawpoolmaterials.cpp.o newview/CMakeFiles/mare-viewer.dir/llface.cpp.o newview/CMakeFiles/mare-viewer.dir/llvovolume.cpp.o newview/CMakeFiles/mare-viewer.dir/pipeline.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted vertex-buffer-heavy `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

The remaining `llrender` public header with direct `llgl.h` exposure is:

- `indra/llrender/llglstates.h`
