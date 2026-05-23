# Viewer Tex Layer Needs Render Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

This packet follows
`docs/architecture/266-viewer-texlayer-needs-render-task.md`.

The source change is limited to the viewer-side
`LLViewerTexLayerSetBuffer` owner.

## Source Changes

Split `LLViewerTexLayerSetBuffer::needsRender()` into private predicate
helpers:

- `hasReadyUpdate()`;
- `isAppearanceAnimationBlocked()`;
- `isSkirtBakeBlocked()`;
- `hasRenderableLocalTextureData()`.

`needsRender()` now reads as the same ordered guard chain:

1. valid agent avatar;
2. update requested and ready;
3. appearance animation not blocking;
4. skirt bake not blocked;
5. local texture data available.

## Behavior Preserved

This packet does not change:

- `ORDER_LAST`;
- dynamic texture `preRender(...)`, `render()`, or `postRender(...)`;
- `preRender(false)`;
- `renderTexLayerSet(mBoundTarget)`;
- update timers;
- low-res update policy;
- skirt bake policy;
- local texture availability/finality policy;
- appearance-side render code under `indra/llappearance/`;
- shader or render state behavior.

## Risk

Risk is low.

Why:

- the packet only names existing predicates;
- the guard order is unchanged;
- avatar bake rendering and appearance-side compositing are untouched.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llviewertexlayer.cpp.o -j8
```

Result: passed.

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- source inventory regenerated; scanned 3085 source files;
- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers do not include `llgl.h` directly outside PCH/prefix;
- raw GL scalar type names are confined to `llglheaders.h`;
- whitespace check passed.

## Integration Build

No non-clean `mare-viewer` integration checkpoint was run for this packet.

Reason:

- targeted owner build passed;
- OpenGL guardrails passed;
- this packet only splits viewer-side predicates and does not touch
  appearance-side compositing.

## Next Small Tasks

- Decide whether to map `LLTexLayerSetBuffer::renderTexLayerSet(...)` before
  any appearance-side cleanup.
- Alternatively, move to a map UI rendering boundary if avatar bake should
  stop at the viewer-side predicate split for now.
