# Draw Pool Alpha Emissive Subpass Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets the alpha emissive subpass at the end of each
rendered alpha spatial group.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Extract the emissive subpass into a private `LLDrawPoolAlpha` member helper.

The helper owns:

- dynamic light restore before emissive drawing;
- glow-accumulating blend mode setup;
- non-PBR emissive queue rendering;
- PBR emissive queue rendering;
- rigged emissive queue rendering;
- rigged PBR emissive queue rendering;
- normal alpha blend restore;
- previous shader rebind when an emissive queue was rendered.

## Invariants

Must remain unchanged:

- the subpass is skipped when `depth_only` is true;
- `gPipeline.enableLightsDynamic()` runs before emissive queue draws;
- glow accumulation uses `BF_ZERO`, `BF_ONE`, `BF_ONE`, `BF_ONE`;
- queue order remains non-PBR, PBR, rigged non-PBR, rigged PBR;
- `light_enabled` is set to true only when a queue is rendered;
- `lastShader` is captured before queue rendering;
- normal alpha blend is restored after queue rendering;
- `lastShader->bind()` runs only when `lastShader` is non-null and at least
  one queue was rendered.

## Risk

Risk level: medium.

Reason:

- the change moves a pass-sized block, but does not change queue membership,
  queue order, blend factors, or shader rebind conditions;
- mistakes here can affect glow/bloom alpha behavior.

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

- Not run. This packet only moves the existing alpha emissive subpass body into
  a private owner-local helper and preserves queue order and restore logic.
