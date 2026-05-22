# Local Clip Rect Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/llui/lllocalcliprect.h` so UI clip-rect consumers no
longer receive `llgl.h` through that public UI header.

## Source Changes

- `indra/llui/lllocalcliprect.h`
  - Removed the public `llgl.h` include.
  - Forward-declared `LLGLState`.
  - Replaced the by-value `LLGLState` member with an owned pointer so the GL
    state helper remains implementation-local.
- `indra/llui/lllocalcliprect.cpp`
  - Added implementation-local `llgl.h` and `llrender.h` includes.
  - Constructs the scissor `LLGLState` in the constructor and keeps the same
    destructor ordering: the clip stack is popped and the scissor region is
    updated before the `LLGLState` instance restores the previous state.

## Risk

Medium-low.

This is still a header-boundary change, but it changes the storage strategy of
the RAII state object from by-value to owned pointer. The lifetime ordering is
preserved: construction happens before the constructor body uses the state, and
destruction happens after the destructor body finishes.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfasttimerview.cpp.o newview/CMakeFiles/mare-viewer.dir/llnetmap.cpp.o newview/CMakeFiles/mare-viewer.dir/llscripteditor.cpp.o newview/CMakeFiles/mare-viewer.dir/llworldmapview.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llui`: passed.
- Targeted `LLLocalClipRect` `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

After this packet, direct `llgl.h` includes in headers are limited to:

- `indra/newview/llviewerprecompiledheaders.h`
- `indra/newview/macview_Prefix.h`

Those are precompiled/prefix headers and should be handled as build-boundary
work rather than normal runtime header cleanup.
