# Tex Layer Projection Scope Task

Date: 2026-05-23

Branch: `phase5`

## Scope

This task follows `docs/architecture/268-texlayer-render-contract-map.md`.

The source packet may only touch:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Goal

Wrap the existing `LLTexLayerSetBuffer` projection push/pop pair in an
owner-local RAII scope.

The scope must preserve the current lifetime:

- projection push still starts in `preRenderTexLayerSet()`;
- projection pop still happens from `postRenderTexLayerSet(...)`.

Because the push and pop happen in separate virtual hooks, the scope may be
stored as owner-local state on `LLTexLayerSetBuffer`.

## Behavior To Preserve

Do not change:

- projection matrix values;
- modelview matrix values;
- call order relative to `LLViewerDynamicTexture::preRender(false)`;
- call order relative to `LLViewerDynamicTexture::postRender(success)`;
- `renderTexLayerSet(...)`;
- shader, color mask, blend, flush, or `midRenderTexLayerSet(success)` policy.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f llappearance/CMakeFiles/llappearance.dir/build.make -B llappearance/CMakeFiles/llappearance.dir/lltexlayer.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
