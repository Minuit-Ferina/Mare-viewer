# Draw Pool Alpha Group Filter Summary

Date: 2026-05-22

## Scope

This packet continues the local `LLDrawPoolAlpha` cleanup.

It names the spatial-group filtering and alpha draw-map selection used by
`LLDrawPoolAlpha::renderAlpha(...)`.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added implementation-local helpers:

- `is_renderable_alpha_group(...)`
- `is_alpha_group_on_rendered_side_of_water(...)`
- `get_alpha_draw_info(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- `mRenderByGroup` and dead-group filtering;
- HUD behavior that ignores above/below-water filtering;
- bridge spatial extents versus group extents selection;
- above-water group rejection condition;
- below-water group rejection condition;
- rigged alpha draw-map selection;
- non-rigged alpha draw-map selection;
- cull-disable behavior for particle and HUD-particle groups;
- per-face render loop order.

No shader selection, blend state, depth state, cull state, or draw call was
changed.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- `lldrawpoolalpha.cpp.o`: passed.
- Generated source inventory: refreshed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. The patch only names existing group filtering and draw-map
  selection decisions.
