# Dynamic Texture Class Helper Summary

Date: 2026-05-22

## Scope

This packet continues the local `LLViewerDynamicTexture::updateAllInstances()`
cleanup.

It moves the dynamic-texture validation and update helpers from
implementation-local lambdas/functions into private static
`LLViewerDynamicTexture` helper methods.

## Source Change

Changed:

- `indra/newview/lldynamictexture.h`
- `indra/newview/lldynamictexture.cpp`
- `docs/architecture/generated/source_inventory.csv`

Added private static helper declarations:

- `validateDynamicTextureTargets(...)`
- `updateDynamicTexture(...)`
- `updateDynamicTextureRange(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- preview target validation before render setup;
- bake target validation before render setup;
- shader unbind before dynamic texture rendering;
- vertex-buffer unbind before dynamic texture rendering;
- preview group orders: `ORDER_FIRST` through before `ORDER_LAST`;
- bake group orders: `ORDER_LAST` through before `ORDER_COUNT`;
- preview target flush before bake target binding;
- bake target clear before bake-range rendering;
- bake target flush before final `gGL.flush()`;
- per-texture `needsRender()` behavior;
- per-texture `preRender()`, `render()`, and `postRender(...)` ordering;
- `sNumRenders` counting;
- existing final return semantics.

Important: the final `ret` value is still assigned from the bake range after
the preview range has already flushed. This preserves the pre-existing behavior
where the final return reflects the bake-target group.

## Header Impact

The new helper declarations are private static methods.

No public method was added, no virtual method was changed, and no data member
was added.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llviewerdisplay.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewertexlayer.cpp.o newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o newview/CMakeFiles/mare-viewer.dir/llgltfmaterialpreviewmgr.cpp.o newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o newview/CMakeFiles/mare-viewer.dir/llfloaterimagepreview.cpp.o newview/CMakeFiles/mare-viewer.dir/llfloaterbvhpreview.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewerjointmesh.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- `lldynamictexture.cpp.o`: passed.
- Direct `lldynamictexture.h` consumer object compiles: passed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Generated source inventory: refreshed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. The patch only moves existing helper logic into private class
  helpers and preserves call order.
