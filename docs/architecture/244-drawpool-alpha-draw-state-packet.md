# Draw Pool Alpha Draw State Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets the per-draw alpha state block in
`LLDrawPoolAlpha::renderAlpha(...)`.

The intended source change is local to `indra/newview/lldrawpoolalpha.cpp`.

## Task

Extract local helpers for:

- applying per-draw alpha blend factors;
- deciding whether minimum alpha must be temporarily lowered to `0`;
- restoring minimum alpha after the draw;
- issuing the vertex-buffer draw call.

## Invariants

Must remain unchanged:

- blend factors come from `params.mBlendFuncSrc`, `params.mBlendFuncDst`,
  `mAlphaSFactor`, and `mAlphaDFactor`;
- the minimum-alpha override is skipped during impostor rendering;
- the minimum-alpha override is skipped when either blend factor is
  `LLRender::BF_SOURCE_ALPHA`;
- `current_shader->setMinimumAlpha(0.f)` happens before the draw when needed;
- `current_shader->setMinimumAlpha(MINIMUM_ALPHA)` happens after the draw when
  needed;
- `params.mVertexBuffer->setBuffer()` stays before `drawRange(...)`;
- `stop_glerror()` stays immediately after the draw call;
- emissive queueing and `RestoreTexSetup(...)` remain after the draw block.

## Risk

Risk level: medium.

Reason:

- this is still a behavior-preserving helper extraction, but the block owns
  per-draw blend state and temporary shader alpha state;
- ordering mistakes here can cause visible alpha or glow differences.

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

- Not run. This packet only extracts the existing per-draw alpha state and draw
  call sequence into local helpers.
