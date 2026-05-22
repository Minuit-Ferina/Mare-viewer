# Phase 3 Source Checkpoint

Date: 2026-05-22

## Scope

This checkpoint records the first source patch after the autonomous phase 3
documentation checkpoint.

## Source Commit

- `d871254617` ui: extract clip rect scissor box calculation

## Source Change

Changed:

- `indra/llui/lllocalcliprect.cpp`

The patch extracts the scissor rectangle math into an implementation-local
helper:

- `compute_scissor_box(...)`

No UI clipping behavior was intentionally changed.

## Why This Was The First Source Patch

This was the lowest-risk source candidate from the narrow abstraction notes:

- one owner file;
- no public API change;
- no header change;
- no render target, shader, draw-pool, or pipeline behavior;
- exact preservation of scissor update order and math.

The dynamic texture and alpha draw-pool candidates were not implemented because
their task notes identified behavior that should be observed or validated
before source extraction.

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

- `llui` targeted build: passed.
- `newview` consumer object compiles: passed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. The patch is a private helper extraction with unchanged scissor
  math and unchanged call order.

## Current Stop Point

Do not continue into larger source changes automatically.

Source areas that should wait for an explicit task or validation:

- `LLViewerDynamicTexture::updateAllInstances()`
- `LLDrawPoolAlpha`
- shader manager behavior
- draw-pool pass ordering
- pipeline behavior
- FSR2 implementation on a non-Darwin build
