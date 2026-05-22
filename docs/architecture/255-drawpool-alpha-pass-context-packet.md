# Draw Pool Alpha Pass Context Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets pass-level parameters threaded through
`LLDrawPoolAlpha::renderAlpha(...)` and `renderAlphaGroup(...)`.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Introduce an owner-local `AlphaPassContext` structure for:

- rigged/non-rigged pass flag;
- depth-only flag;
- selected water side;
- current water height.

## Invariants

Must remain unchanged:

- `rigged` still selects the same alpha group iterator and draw map;
- `depth_only` still suppresses only the alpha emissive subpass;
- water-side filtering still uses the same `above_water` value;
- water-side filtering still uses `LLEnvironment::getWaterHeight()`;
- no shader, texture, draw, or emissive queue behavior changes.

## Risk

Risk level: low.

Reason:

- this replaces repeated pass-level parameters with one owner-local context
  object;
- the context is built from the same values immediately before traversal.

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

- Not run. This packet only replaces repeated pass-level parameters with an
  owner-local context structure.
