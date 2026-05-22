# LLGL Header Type Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows the remaining raw GL scalar type spelling in
`indra/llrender/llgl.h`.

`llgl.h` remains the intentional low-level OpenGL state boundary. This change
only replaces the public `GLboolean` spelling on `LLGLState::checkStates(...)`
with the existing project-owned alias.

## Source Changes

- `indra/llrender/llgl.h`
  - Replaced `LLGLState::checkStates(GLboolean ...)` with
    `LLGLState::checkStates(LLGLboolean ...)`.
- `indra/llrender/llgl.cpp`
  - Matched the method definition to the updated alias spelling.

## Risk

Low.

`LLGLboolean` has the same underlying scalar representation used elsewhere in
the GL containment work. The callsites and default value are unchanged.

## Verification

Commands run:

```sh
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/pipeline.cpp.o newview/CMakeFiles/mare-viewer.dir/llspatialpartition.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `llrender/fast`: passed.
- `llui`: passed.
- Targeted `newview` object compiles using `LLGLState::checkStates(...)`:
  passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

The only remaining raw GL declarations in `llrender` headers are now in
`llglheaders.h`, the platform GL loader/header boundary.
