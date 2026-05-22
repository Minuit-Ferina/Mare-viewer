# Phase 3 Small Source Cleanups Checkpoint

Date: 2026-05-22

## Scope

This checkpoint summarizes the small source cleanups performed after the phase
3 source checkpoint.

All changes are intended to preserve runtime behavior.

## Source Commits

- `d871254617` ui: extract clip rect scissor box calculation
- `ba21bfd690` render: clarify dynamic texture update result flow
- `8698123b85` render: name alpha particle cull predicate
- `c66c3be4a1` render: name dynamic texture target validation
- `aa4bc2257c` render: name dynamic texture range update
- `bf7642c379` docs: refresh source inventory after source cleanups

## Source Areas Touched

`indra/llui/lllocalcliprect.cpp`:

- extracted scissor box coordinate math into an implementation-local helper;
- preserved flush, error checks, scissor call, stack behavior, and
  `LLGLState` lifetime.

`indra/newview/lldynamictexture.cpp`:

- extracted preview/bake target validation;
- made the per-texture update lambda return the local render result;
- extracted repeated order-range iteration;
- preserved preview/bake ordering and existing final return semantics.

`indra/newview/lldrawpoolalpha.cpp`:

- named the existing particle/HUD-particle group predicate used to disable
  culling;
- preserved alpha shader, blend, depth, water, and emissive behavior.

## Verification Used

Targeted builds:

```sh
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfasttimerview.cpp.o newview/CMakeFiles/mare-viewer.dir/llscripteditor.cpp.o newview/CMakeFiles/mare-viewer.dir/llnetmap.cpp.o newview/CMakeFiles/mare-viewer.dir/llworldmapview.cpp.o newview/CMakeFiles/mare-viewer.dir/llsnapshotlivepreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Current guardrail status:

- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers still avoid direct `llgl.h` includes outside PCH/prefix;
- raw GL scalar type names remain confined to `llglheaders.h`;
- generated source inventory is refreshed.

## Runtime Smoke

No runtime smoke was run for this checkpoint.

Reason:

- these patches are local helper extractions and predicate naming;
- no call order, pass order, shader selection, blend/depth state, or scissor
  math was intentionally changed.

## Stop Point

Further source changes should not broaden into:

- `LLDrawPoolAlpha` blend/depth/water behavior;
- `LLViewerDynamicTexture` return semantics or special user behavior;
- shader manager behavior;
- draw-pool pass ordering;
- `pipeline.cpp` pass orchestration;
- FSR2 implementation without a non-Darwin `MARE_ENABLE_FSR2=ON` validation
  environment.
