# Draw Pool Alpha Debug Render Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets `LLDrawPoolAlpha::renderDebugAlpha(...)`.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Extract owner-local helpers for:

- rendering static debug alpha highlight batches;
- rendering rigged debug alpha highlight batches.

## Invariants

Must remain unchanged:

- debug alpha still renders only when `sShowDebugAlpha && !gCubeSnapshot`;
- initial highlight shader bind remains before smoke texture setup;
- smoke texture bind remains before `renderAlphaHighlight()`;
- static debug batch order and colors remain unchanged;
- rigged debug batch order and colors remain unchanged;
- final current shader unbind remains after both debug halves.

## Risk

Risk level: low.

Reason:

- this only names two already-linear debug rendering halves;
- no batch IDs, colors, shader binds, or texture binds change.

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

- Not run. This packet only extracts existing debug alpha rendering halves into
  owner-local helpers.
