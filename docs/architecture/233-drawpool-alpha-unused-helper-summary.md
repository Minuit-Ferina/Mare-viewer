# Draw Pool Alpha Unused Helper Summary

Date: 2026-05-22

## Scope

This packet removes unused implementation-local helper functions from
`LLDrawPoolAlpha`.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Removed unused helpers:

- `IsFullbright(...)`
- `IsMaterial(...)`
- `IsEmissive(...)`
- `Draw(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

These helpers had no callsites in `lldrawpoolalpha.cpp`.

No render path, shader binding, texture binding, draw call, pass order, or
state lifetime was changed.

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

- Not run. The patch removes unused implementation-local code only.
