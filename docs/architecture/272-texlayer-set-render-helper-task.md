# Tex Layer Set Render Helper Task

Date: 2026-05-23

Branch: `phase5`

## Scope

This task follows `docs/architecture/271-texlayer-set-render-map.md`.

The source packet may only touch:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Goal

Split `LLTexLayerSet::render(...)` into private owner-local helpers without
changing render behavior.

Allowed helpers:

- `hasInvisibleAlphaMask() const`;
- `clearCompositeBuffer(S32 width, S32 height)`;
- `renderColorLayers(S32 x, S32 y, S32 width, S32 height,
  LLRenderTarget* bound_target)`;
- `clearInvisibleComposite(S32 width, S32 height)`.

## Behavior To Preserve

Do not change:

- `mIsVisible` reset and invisible-mask policy;
- `LLGLSUIDefault` scope;
- `LLGLDepthTest(GL_FALSE, GL_FALSE)` scope;
- color mask policy;
- alpha minimum values;
- texture unit 0 unbind placement;
- draw rectangle dimensions;
- flush placement;
- color-pass layer ordering;
- `bound_target` propagation;
- `success` accumulation;
- alpha-mask render timing;
- blend restore timing on the invisible path.

## Risk

Risk is medium.

Why:

- the code touches direct render state through `gGL` and shader state;
- this packet is mechanically small, but the exact ordering is important.

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
