# Draw Pool Alpha Particle Cull Summary

Date: 2026-05-22

## Scope

This packet applies a minimal source cleanup within the boundaries defined by
`docs/architecture/220-drawpool-alpha-state-task.md`.

It does not change alpha pass order, shaders, blend factors, depth decisions,
water behavior, or texture setup.

## Source Change

Changed:

- `indra/newview/lldrawpoolalpha.cpp`

Added an implementation-local predicate:

- `is_particle_or_hud_particle_group(LLSpatialGroup* group)`

The predicate names the existing condition used to disable face culling for
particle and HUD particle alpha groups.

## Behavior Preserved

Preserved:

- same partition types:
  - `LLViewerRegion::PARTITION_PARTICLE`
  - `LLViewerRegion::PARTITION_HUD_PARTICLE`
- same `LLGLDisable cull(...)` behavior;
- same render group loop;
- same alpha shader, blend, depth, water, and emissive behavior.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- `lldrawpoolalpha.cpp.o`: passed.
- GL containment guardrail: passed.
- Header boundary guardrail: passed.
- Diff whitespace check: passed.

Runtime smoke:

- Not run. The patch names an existing predicate and leaves render state
  behavior unchanged.
