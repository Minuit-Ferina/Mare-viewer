# Draw Pool Alpha Group Pipeline Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets the spatial-group body inside
`LLDrawPoolAlpha::renderAlpha(...)`.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Extract an owner-local helper for rendering one alpha spatial group.

The helper owns:

- group assertions;
- renderable-group filtering;
- water-side filtering;
- per-group emissive queue storage and clearing;
- particle/HUD particle cull state;
- draw-info traversal for the current rigged/non-rigged pass;
- alpha emissive subpass for the accepted group.

## Invariants

Must remain unchanged:

- group profiling label remains `renderAlpha - group`;
- group and spatial-partition assertions still run for each traversed group;
- non-renderable groups are skipped;
- water-side filtering still happens before queue clearing and draw traversal;
- emissive queue storage remains static at group scope;
- particle/HUD particle groups still disable culling for the whole group body;
- draw-info filtering by rigged pass remains before per-draw rendering;
- emissive subpass still runs after all accepted draw info in the group and
  only when `depth_only` is false.

## Risk

Risk level: medium.

Reason:

- this moves one full group body, but keeps the draw pipeline helper and
  emissive subpass order intact;
- group-level state scopes, especially culling, must remain unchanged.

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

- Not run. This packet extracts the existing alpha spatial-group body into an
  owner-local helper and preserves cull scope and subpass ordering.
