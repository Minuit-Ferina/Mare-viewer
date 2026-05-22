# Draw Pool Alpha Draw Pipeline Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets the body of `LLDrawPoolAlpha::renderAlpha(...)`.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Extract owner-local structure for:

- the four alpha emissive queues used by a spatial group;
- clearing and queueing alpha emissive draw info;
- rendering the alpha emissive subpass from that queue structure;
- rendering one alpha draw info, including shader setup, matrix palette upload,
  texture setup, draw, emissive queueing, and texture restore.

## Invariants

Must remain unchanged:

- emissive queue storage remains static per `renderAlpha(...)` group traversal;
- queue clearing still happens once per accepted spatial group;
- queue membership still depends on avatar presence and GLTF material presence;
- GLTF blend shader bind still happens before GLTF material bind;
- non-GLTF shader/material/fullbright/HUD setup remains unchanged;
- matrix-palette upload still happens after shader selection and before
  texture setup;
- failed matrix-palette upload still skips the draw;
- `TexSetup(...)`, `render_alpha_batch(...)`, emissive queueing, and
  `RestoreTexSetup(...)` keep their relative order;
- emissive subpass queue order remains non-PBR, PBR, rigged non-PBR, rigged PBR.

## Risk

Risk level: medium.

Reason:

- this moves a larger block, but the extracted method preserves the existing
  per-draw order;
- the disabled RLV/PBR block remains preserved near the non-GLTF alpha path.

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

- Not run. This packet extracts existing alpha draw pipeline behavior into
  owner-local helpers and preserves the per-draw order.
