# Draw Pool Alpha Pass Decision Summary

Date: 2026-05-22

## Scope

This packet continues the `LLDrawPoolAlpha` source cleanup started by the
particle/HUD-particle cull predicate extraction.

It names several local alpha-pass decisions without changing draw order, shader
selection, blend factors, or render-state lifetime.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`
- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

Added implementation-local helpers:

- `get_alpha_water_sign(...)`
- `should_render_alpha_depth_of_field_pass(...)`
- `should_write_alpha_depth(...)`
- `render_gltf_scene_depth_for_rigged_alpha()`

The generated source inventory was refreshed after the source shape changed.

## Behavior Preserved

Preserved:

- early water-clip return for pre-water alpha;
- underwater sign inversion;
- shader preparation order;
- explicit shader unbind before forward alpha rendering;
- rigged depth pass before regular alpha;
- depth-of-field depth-only pass condition;
- color-mask changes around the depth-of-field depth-only pass;
- alpha depth-write condition;
- GLTF scene depth render order before rigged post-water alpha;
- normal alpha blend-factor setup;
- final debug alpha execution inside the non-rigged alpha state scope.

No pass ID, pool order, shader object, blend factor, or draw loop was changed.

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

- Not run. The patch only names existing local decisions and preserves
  call order.
