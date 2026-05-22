# Postprocess And Sphere Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows two low-risk `indra/llrender/` public headers that pulled
in `llgl.h` for transitive declarations rather than direct public OpenGL
contracts:

- `indra/llrender/llpostprocess.h`
- `indra/llrender/llrendersphere.h`

## Source Changes

- `indra/llrender/llpostprocess.h`
  - Removed the public `llgl.h` include.
  - Added direct includes for `llgltypes.h`, `llpointer.h`, `llsd.h`, and
    public STL types used by the header.
  - Forward-declared `LLImageGL` for pointer members and helper signatures.
- `indra/llrender/llpostprocess.cpp`
  - Added the implementation-local `llimagegl.h` include for texture object
    construction and static texture helpers.
- `indra/llrender/llrendersphere.h`
  - Removed the public `llgl.h` include.
  - Added direct includes for `llpointer.h` and `<vector>`.
  - Forward-declared `LLVertexBuffer` for the retained pointer member.
- `indra/llrender/llrendersphere.cpp`
  - Added the implementation-local `llrender.h` include for `gGL` and
    `LLRender` usage.

## Risk

Low.

The public class layouts and function signatures are unchanged. The main risk
is compile fallout from consumers that relied on either header to provide
unrelated GL or renderer declarations. Targeted `newview` object compiles cover
the highest-value direct consumers.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llviewerwindow.cpp.o newview/CMakeFiles/mare-viewer.dir/lldrawpoolavatar.cpp.o newview/CMakeFiles/mare-viewer.dir/llfloaterpostprocess.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted sphere/postprocess `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

The remaining `llrender` public headers with direct `llgl.h` exposure are
heavier and should remain separate packets:

- `indra/llrender/llgltexture.h`
- `indra/llrender/llrendertarget.h`
- `indra/llrender/llvertexbuffer.h`
- `indra/llrender/llglstates.h`
