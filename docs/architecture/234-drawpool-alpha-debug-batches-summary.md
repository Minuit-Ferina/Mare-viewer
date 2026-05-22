# Draw Pool Alpha Debug Batches Summary

Date: 2026-05-22

## Scope

This packet continues the local `LLDrawPoolAlpha` cleanup.

It names the static and rigged debug alpha-highlight batch groups used by
`renderDebugAlpha()`.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added implementation-local helpers:

- `push_static_alpha_highlight_mask_batches(...)`
- `push_static_material_alpha_highlight_batches(...)`
- `push_rigged_alpha_highlight_mask_batches(...)`
- `push_rigged_material_alpha_highlight_batches(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- debug alpha enable condition;
- cube-snapshot exclusion;
- highlight shader binding order;
- red, blue, and green debug colors;
- static alpha-mask batch order;
- static material alpha-mask batch order;
- static invisible batch order;
- rigged alpha-mask batch order;
- rigged material alpha-mask batch order;
- rigged invisible batch order;
- final shader unbind.

No batch ID, color, shader, texture, or draw order was changed.

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

- Not run. The patch only groups existing debug alpha-highlight batch calls.
