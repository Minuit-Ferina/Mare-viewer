# Draw Pool Alpha Pass State Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets the state threaded through
`LLDrawPoolAlpha::renderAlpha(...)`, `renderAlphaGroup(...)`, and
`renderAlphaDraw(...)`.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Introduce an owner-local `AlphaRenderState` structure for:

- cached avatar pointer;
- cached mesh id;
- cached avatar shader pointer;
- skip-last-skin flag;
- initialized-lighting flag;
- light-enabled flag.

## Invariants

Must remain unchanged:

- all fields start with the same initial values as the former local variables;
- matrix-palette upload receives and mutates the same cached state;
- non-GLTF lighting setup mutates the same lighting state;
- final alpha render restore still uses the current light-enabled state;
- emissive subpass still sets light-enabled to true when it renders a queue.

## Risk

Risk level: low.

Reason:

- this replaces repeated reference arguments with one owner-local state object;
- field initialization and mutation order remain unchanged.

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

- Not run. This packet only replaces repeated pass-state reference arguments
  with an owner-local state structure.
