# Draw Pool Alpha Highlight Draw Summary

Date: 2026-05-22

## Scope

This packet continues the local `LLDrawPoolAlpha` cleanup.

It names the per-draw debug alpha-highlight rendering helper and reuses the
existing alpha group renderability predicate in `renderAlphaHighlight()`.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added implementation-local helper:

- `draw_alpha_highlight_info(...)`

Reused existing helper:

- `is_renderable_alpha_group(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- dead-group and `mRenderByGroup` filtering;
- rigged highlight shader selection from `params.mAvatar`;
- matrix-palette upload before rigged highlight draw;
- failed matrix-palette upload skip behavior;
- red highlight color before each highlight draw;
- model-matrix application;
- vertex-buffer bind;
- highlight draw range.

No shader object, color, draw call, draw range, or group iteration order was
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

- Not run. The patch only names existing debug alpha-highlight per-draw logic.
