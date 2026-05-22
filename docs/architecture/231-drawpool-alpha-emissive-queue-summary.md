# Draw Pool Alpha Emissive Queue Summary

Date: 2026-05-22

## Scope

This packet continues the local `LLDrawPoolAlpha` cleanup.

It names the alpha emissive/glow queueing decision and reuses the existing
texture-matrix restore helper in the main alpha draw loop.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added implementation-local helpers:

- `should_queue_alpha_emissive(...)`
- `queue_alpha_emissive(...)`

Reused existing member helper:

- `RestoreTexSetup(...)`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- pre-water alpha meshes are not queued for emissive/glow second pass;
- emissive vertex-buffer test;
- rigged emissive queue selection;
- non-rigged emissive queue selection;
- PBR emissive queue selection;
- non-PBR emissive queue selection;
- texture matrix restore operations and order;
- per-face draw order.

No emissive render call, blend state, shader binding, or texture binding
behavior was changed.

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

- Not run. The patch only names existing emissive queueing logic and reuses an
  existing helper with identical texture-matrix restore calls.
