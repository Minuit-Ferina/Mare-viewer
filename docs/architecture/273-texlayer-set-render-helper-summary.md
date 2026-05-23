# Tex Layer Set Render Helper Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

This packet follows
`docs/architecture/272-texlayer-set-render-helper-task.md`.

The source change is limited to the appearance-side `LLTexLayerSet` owner:

- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Source Changes

Split `LLTexLayerSet::render(...)` into private owner-local helpers:

- `hasInvisibleAlphaMask() const`;
- `clearCompositeBuffer(S32 width, S32 height)`;
- `renderColorLayers(S32 x, S32 y, S32 width, S32 height,
  LLRenderTarget* bound_target)`;
- `clearInvisibleComposite(S32 width, S32 height)`.

`render(...)` now reads as the same ordered render sequence:

1. initialize success;
2. update `mIsVisible` from invisible alpha masks;
3. enter existing UI/default and depth-state scopes;
4. enable color and alpha writes;
5. clear the composite buffer;
6. render visible color layers and alpha masks, or clear invisible output;
7. return accumulated success.

## Behavior Preserved

This packet does not change:

- `mIsVisible` policy;
- mask-layer scan coverage;
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
- invisible-path blend restore timing.

## Risk

Risk is medium.

Why:

- this function still controls render state directly;
- the helper split is owner-local, but exact ordering matters;
- no state ownership was moved outside `LLTexLayerSet`.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f llappearance/CMakeFiles/llappearance.dir/build.make -B llappearance/CMakeFiles/llappearance.dir/lltexlayer.cpp.o -j8
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
- this packet only names existing `LLTexLayerSet::render(...)` blocks as
  private helpers.

## Next Small Tasks

- Decide whether to map `LLTexLayer::render(...)` next.
- Alternatively, switch to a UI rendering owner map if appearance compositing
  should stop at the `LLTexLayerSet` boundary for now.
