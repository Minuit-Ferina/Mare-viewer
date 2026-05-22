# Dynamic Texture Range Update Summary

Date: 2026-05-22

## Scope

This packet continues the local `LLViewerDynamicTexture::updateAllInstances()`
cleanup.

It extracts the repeated order-range iteration into a local lambda while
preserving the existing preview/bake group ordering and final return semantics.

## Source Change

Changed:

- `indra/newview/lldynamictexture.cpp`

Added a local lambda:

- `update_dynamic_texture_range(...)`

The lambda iterates an order range, calls the existing per-texture lambda, and
returns whether any dynamic texture in that range rendered.

## Behavior Preserved

Preserved:

- preview group orders: `ORDER_FIRST` through before `ORDER_LAST`;
- bake group orders: `ORDER_LAST` through before `ORDER_COUNT`;
- preview target flush before bake target binding;
- bake target flush before final `gGL.flush()`;
- per-texture `needsRender()` behavior;
- per-texture render result handling;
- `sNumRenders` counting;
- existing final return semantics.

Important: the final `ret` value is assigned from the bake range after the
preview range has already flushed. This preserves the pre-existing behavior
where the final return reflects the bake-target group.

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

- Not run. The patch preserves ordering and local return semantics.
