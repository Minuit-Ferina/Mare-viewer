# Draw Pool Alpha Traversal Packet

Date: 2026-05-23

## Scope

This phase 4 packet targets the outer traversal scaffolding in
`LLDrawPoolAlpha::renderAlpha(...)`.

The intended source change is local to `indra/newview/lldrawpoolalpha.cpp`.

## Task

Extract local helpers for:

- selecting alpha spatial-group begin/end iterators;
- deciding the rendered side of water for the current alpha pool;
- filtering draw info by rigged/non-rigged pass;
- restoring final alpha render state after all groups are processed.

## Invariants

Must remain unchanged:

- rigged pass uses `beginRiggedAlphaGroups()` and `endRiggedAlphaGroups()`;
- non-rigged pass uses `beginAlphaGroups()` and `endAlphaGroups()`;
- `POOL_ALPHA_POST_WATER` is the base above-water test;
- underwater rendering still flips the above-water side;
- draw info is skipped when `(bool)params.mAvatar != rigged`;
- final scene blend type is restored to `LLRender::BT_ALPHA`;
- `LLVertexBuffer::unbind()` stays after all alpha groups;
- dynamic lights are restored at the end only when `light_enabled` is false.

## Risk

Risk level: low.

Reason:

- this packet only names branch decisions and final restore code;
- it does not move shader selection, texture setup, draw calls, emissive queue
  membership, or pass ordering.

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

- Not run. This packet only extracts traversal and final-restore decisions into
  local helpers.
