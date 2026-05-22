# Draw Pool Alpha Highlight Summary

Date: 2026-05-22

## Scope

This packet continues the local `LLDrawPoolAlpha` cleanup.

It names the debug alpha-highlight group selection and preserves the existing
`PASS_ALPHA + pass` mapping for the second rigged highlight pass.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added implementation-local helpers:

- `is_alpha_highlight_rigged_pass(...)`
- `begin_alpha_highlight_groups(...)`
- `end_alpha_highlight_groups(...)`
- `get_alpha_highlight_draw_info(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- two alpha-highlight passes;
- first pass uses non-rigged alpha groups;
- second pass uses rigged alpha groups;
- existing `PASS_ALPHA + pass` draw-map mapping;
- per-draw rigged shader binding;
- matrix-palette upload behavior;
- highlight draw order.

No debug color, shader, vertex buffer, draw call, or model-matrix behavior was
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

- Not run. The patch only names existing debug alpha-highlight selection logic.
