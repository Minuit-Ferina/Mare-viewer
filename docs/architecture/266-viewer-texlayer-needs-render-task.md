# Viewer Tex Layer Needs Render Task

Date: 2026-05-23

Branch: `phase5`

## Scope

This task follows `docs/architecture/265-viewer-texlayer-buffer-owner-map.md`.

The source packet may only touch:

- `indra/newview/llviewertexlayer.h`
- `indra/newview/llviewertexlayer.cpp`

## Goal

Split `LLViewerTexLayerSetBuffer::needsRender()` into private owner-local
predicate helpers.

This is a readability and ownership cleanup only. It must not change avatar
bake policy or render behavior.

## Behavior To Preserve

`needsRender()` must still return false unless:

1. the agent avatar is valid;
2. an update is requested and ready;
3. the avatar is not appearance-animating;
4. the requested bake is not the invalid skirt case;
5. local texture data is available.

## Not Allowed

Do not change:

- `ORDER_LAST`;
- `preRender(false)`;
- `renderTexLayerSet(mBoundTarget)`;
- `postRenderTexLayerSet(success)`;
- update timers;
- low-res update policy;
- skirt bake policy;
- local texture availability/finality policy;
- `LLTexLayerSetBuffer::renderTexLayerSet(...)`;
- any file under `indra/llappearance/`.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llviewertexlayer.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
