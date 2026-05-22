# Pipeline Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows `indra/newview/pipeline.h` so pipeline consumers no longer
receive `llgl.h` through the broad public pipeline header.

## Source Changes

- `indra/newview/pipeline.h`
  - Removed the public `llgl.h` include.
  - Added a forward declaration for `LLGLUpdate`, which is only referenced as a
    pointer in the public API.
- `indra/newview/pipeline.cpp`
  - Added the implementation-local `llgl.h` include for `LLGLUpdate`,
    `LLGLState`, `LLGLEnable`, `LLGLDisable`, `LLGLDepthTest`, GL manager
    access, and diagnostics used by the pipeline implementation.

## Risk

Medium.

`pipeline.h` is a broad viewer header. The change is intentionally limited to
include ownership: the render-target members, shader APIs, draw pools, and
runtime pipeline behavior are unchanged.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/pipeline.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewerdisplay.cpp.o newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewerwindow.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- Targeted pipeline-heavy `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

Remaining direct `llgl.h` includes outside the intentional low-level boundary:

- `indra/llui/lllocalcliprect.h`: owns `LLGLState` by value.
- `indra/newview/marefsr2upscaler.h`: Darwin-excluded locally but still raw
  GL-facing in the source tree.
