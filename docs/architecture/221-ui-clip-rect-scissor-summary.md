# UI Clip Rect Scissor Summary

Date: 2026-05-22

## Scope

This packet applies the narrow task from
`docs/architecture/219-ui-clip-rect-scissor-task.md`.

It does not change UI clipping behavior. It extracts only the rectangle and
scale-factor math used to compute the scissor box.

## Source Change

Changed:

- `indra/llui/lllocalcliprect.cpp`

Added an implementation-local helper:

- `compute_scissor_box(const LLRect& rect, S32& x, S32& y, S32& w, S32& h)`

The helper preserves the existing math:

- floor scaled left/bottom;
- ceil scaled width/height;
- clamp width/height at zero;
- add one pixel to width/height.

## Behavior Preserved

Preserved:

- clip stack push/pop behavior;
- empty rectangle normalization;
- `gGL.flush()` before scissor update;
- `stop_glerror()` positions;
- `LLGLContainment::setScissorBox(...)` callsite;
- constructor/destructor ordering;
- `LLGLState` scissor lifetime;
- local-to-screen conversion in `LLLocalClipRect`.

## Verification

Commands run:

```sh
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfasttimerview.cpp.o newview/CMakeFiles/mare-viewer.dir/llscripteditor.cpp.o newview/CMakeFiles/mare-viewer.dir/llnetmap.cpp.o newview/CMakeFiles/mare-viewer.dir/llworldmapview.cpp.o newview/CMakeFiles/mare-viewer.dir/llsnapshotlivepreview.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- `llui`: passed.
- Targeted `newview` consumer object compiles: passed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. The patch is a local helper extraction with unchanged call order and
  unchanged scissor math.
