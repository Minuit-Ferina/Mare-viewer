# Draw Pool Alpha Emissive Draw Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets the per-draw emissive helpers used by the alpha
emissive subpass.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Extract owner-local helpers for:

- legacy emissive draw setup, draw, and texture restore;
- PBR emissive material bind, cull state, and draw call.

## Invariants

Must remain unchanged:

- legacy emissive draws still set emissive brightness to `1.f`;
- legacy emissive draws still use `TexSetup(draw, false)`;
- legacy emissive draws still call `RestoreTexSetup(...)` after drawing;
- PBR emissive draws still assert `mGLTFMaterial`;
- PBR emissive draws still disable culling when the GLTF material is
  double-sided;
- PBR emissive draws still bind the GLTF material before setting and drawing
  the vertex buffer;
- rigged emissive loops still skip draws when matrix-palette upload fails.

## Risk

Risk level: low.

Reason:

- this is a local duplication cleanup inside already-separated emissive paths;
- shader binding and matrix-palette upload order remain in the caller loops.

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

- Not run. This packet only extracts existing per-draw emissive behavior into
  owner-local helpers.
