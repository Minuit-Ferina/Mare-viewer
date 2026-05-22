# Dynamic Texture Target Validation Summary

Date: 2026-05-22

## Scope

This packet continues the dynamic texture render-scope cleanup in
`indra/newview/lldynamictexture.cpp`.

It extracts only the preview/bake render-target validation from
`LLViewerDynamicTexture::updateAllInstances()`.

## Source Change

Changed:

- `indra/newview/lldynamictexture.cpp`

Added an implementation-local helper:

- `validate_dynamic_texture_targets(LLRenderTarget& preview_target,
  LLRenderTarget& bake_target)`

The helper preserves:

- completeness checks for preview and bake targets;
- the existing `llassert(false)` on incomplete targets;
- the existing `return false` behavior on incomplete targets;
- width/height assertions for preview target and bake target.

## Behavior Preserved

Preserved:

- target selection;
- bind/clear order;
- order bucket iteration;
- per-texture render flow;
- final return semantics;
- `sNumRenders` behavior;
- dynamic texture user behavior.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- `lldynamictexture.cpp.o`: passed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. The patch only extracts existing validation checks.
