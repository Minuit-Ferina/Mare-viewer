# Draw Pool Alpha Forward Render Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets `LLDrawPoolAlpha::forwardRender(...)`.

The intended source change is owner-local:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

## Task

Extract owner-local helpers for:

- forward alpha color-mask and blend-factor setup;
- rigged GLTF depth prepass condition;
- forward alpha finish color-mask and debug-alpha call.

## Invariants

Must remain unchanged:

- `LLGLSPipelineAlpha` and `LLGLDepthTest` RAII objects remain scoped in
  `forwardRender(...)`;
- dynamic lights are enabled before alpha pipeline state is created;
- color writes to alpha remain enabled before depth state and drawing;
- depth write policy still comes from `should_write_alpha_depth(...)`;
- blend factors keep the same color and alpha values;
- rigged GLTF scene depth prepass still runs only for rigged
  `POOL_ALPHA_POST_WATER`;
- normal alpha rendering still uses the shared alpha vertex-data mask;
- final color mask is restored after `renderAlpha(...)`;
- debug alpha still runs only on the final non-rigged pass.

## Risk

Risk level: low.

Reason:

- RAII state lifetimes are intentionally left in `forwardRender(...)`;
- extracted helpers only name existing condition/action blocks.

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

- Not run. This packet names existing `forwardRender(...)` blocks while
  preserving RAII state lifetimes in the original function.
