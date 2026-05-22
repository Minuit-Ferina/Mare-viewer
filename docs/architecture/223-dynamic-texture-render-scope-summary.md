# Dynamic Texture Render Scope Summary

Date: 2026-05-22

## Scope

This packet applies the smallest safe part of
`docs/architecture/218-dynamic-texture-render-scope-task.md`.

It does not extract a new class or change dynamic texture behavior. It only
turns the local per-instance update lambda into a boolean-returning helper so
the render result is local to each dynamic texture update.

## Source Change

Changed:

- `indra/newview/lldynamictexture.cpp`

What changed:

- renamed the local update lambda to `update_dynamic_texture`;
- made it return whether that dynamic texture rendered;
- moved the per-instance `result` variable inside the lambda;
- kept `sNumRenders++` tied to `render()` returning true;
- kept `postRender(result)` after clearing the bound target.

## Behavior Preserved

Preserved:

- `needsRender()` skip behavior;
- target selection between preview and bake targets;
- order iteration;
- `preRender()` before `render()`;
- `setBoundTarget(nullptr)` before `postRender(result)`;
- shader and vertex-buffer unbind positions;
- depth clear and white color setup before each render;
- preview target flush before bake target work;
- bake target flush before final `gGL.flush()`;
- existing final return semantics.

Important: `ret` is still reset to `false` before the bake-target group. The
function still returns the bake-target group result, matching the pre-existing
behavior documented in the task note.

## Verification

Commands run:

```sh
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- `llrender/fast`: passed/already up to date.
- `lldynamictexture.cpp.o`: passed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. This is a local helper-shape cleanup with preserved ordering and
  preserved return semantics.

## Follow-Up

Do not change dynamic texture return semantics, `LLViewerTexLayerSetBuffer`,
`LLVisualParamReset`, or `LLPreviewAnimation` behavior without a dedicated
behavior task and runtime validation.
