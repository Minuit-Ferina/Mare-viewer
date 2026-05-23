# Tex Layer Set Render Map

Date: 2026-05-23

Branch: `phase5`

## Scope

This note maps `LLTexLayerSet::render(...)` before any source cleanup inside
the deeper appearance-side compositing owner.

No source files are modified by this note.

The immediate owner is:

- `LLTexLayerSet::render(...)` in `indra/llappearance/lltexlayer.cpp`

## Inputs Inspected

- `AGENTS.md`
- `docs/architecture/268-texlayer-render-contract-map.md`
- `docs/architecture/270-texlayer-projection-scope-summary.md`
- `docs/architecture/generated/source_inventory.csv`
- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Inventory Snapshot

Current generated inventory signals:

| file | category | lines | `gGL` | `LLGL` | `LLRender` | `LLRenderTarget` |
|---|---|---:|---:|---:|---:|---:|
| `indra/llappearance/lltexlayer.cpp` | `render.opengl_touching` | 1957 | 85 | 20 | 17 | 10 |
| `indra/llappearance/lltexlayer.h` | `render.opengl_touching` | 320 | 0 | 2 | 0 | 12 |

There are no direct runtime `gl*` calls here after phase 3 containment, but
`LLTexLayerSet::render(...)` still directly controls render state through
`gGL`, `LLGLSUIDefault`, `LLGLDepthTest`, `gAlphaMaskProgram`, and
`LLRender::BT_*` blend modes.

## Owner Boundary

`LLTexLayerSet::render(...)` owns:

- resetting `mIsVisible` for the current composite render;
- detecting invisible alpha masks;
- setting UI render state and disabled depth writes/tests for compositing;
- clearing the target rectangle before layer compositing;
- rendering color-pass layers in list order;
- invoking `renderAlphaMaskTextures(...)` after color layers;
- clearing to transparent output when invisible masks hide the set;
- returning the accumulated layer render success.

It does not own:

- dynamic texture target setup;
- projection setup/restore;
- final `LLTexLayerSetBuffer::renderTexLayerSet(...)` shader and cleanup
  policy;
- individual `LLTexLayer::render(...)` behavior;
- individual alpha parameter rendering;
- morph mask gathering or application.

## Current Render Flow

`LLTexLayerSet::render(...)` currently:

1. Initializes `success` to true.
2. Sets `mIsVisible` to true.
3. Scans `mMaskLayerList` for invisible alpha masks and may set
   `mIsVisible` to false.
4. Enters `LLGLSUIDefault`.
5. Enters `LLGLDepthTest(GL_FALSE, GL_FALSE)`.
6. Enables color and alpha writes.
7. Clears the composite area:
   - flush;
   - set alpha minimum to `0.0f`;
   - unbind texture unit 0;
   - set opaque black;
   - draw `gl_rect_2d_simple(width, height)`;
   - flush;
   - restore alpha minimum to `0.004f`.
8. If visible:
   - for each color-pass layer, flush, render layer, flush;
   - render alpha-mask textures;
   - check `stop_glerror()`.
9. If invisible:
   - flush;
   - set blend to replace;
   - set alpha minimum to `0.0f`;
   - unbind texture unit 0;
   - set transparent black;
   - draw `gl_rect_2d_simple(width, height)`;
   - restore blend to alpha;
   - flush;
   - restore alpha minimum to `0.004f`.
10. Returns `success`.

## State To Preserve

Visibility:

- `mIsVisible` starts true for each render;
- any invisible alpha mask can set it false;
- all mask layers are still scanned.

Render state:

- `LLGLSUIDefault` and `LLGLDepthTest(GL_FALSE, GL_FALSE)` cover the same
  render body;
- color mask is set before the initial clear;
- clear color and alpha values remain unchanged;
- alpha minimum changes remain paired with the same draw calls;
- texture unit 0 unbinds remain in the same clear paths;
- flush placement remains before and after the same draw/layer calls;
- invisible output still uses `LLRender::BT_REPLACE` and restores
  `LLRender::BT_ALPHA`.

Data flow:

- color-pass layers render in existing list order;
- `bound_target` is passed through to each color-pass layer and to
  `renderAlphaMaskTextures(...)`;
- `success` continues to accumulate each color-pass layer render result;
- alpha masks run only on the visible path;
- `success` is returned unchanged after invisible-output cleanup.

## Safe Source Packet Candidate

The next source packet can split `LLTexLayerSet::render(...)` into private
owner-local helpers:

- `hasInvisibleAlphaMask() const`;
- `clearCompositeBuffer(S32 width, S32 height)`;
- `renderColorLayers(S32 x, S32 y, S32 width, S32 height,
  LLRenderTarget* bound_target)`;
- `clearInvisibleComposite(S32 width, S32 height)`.

This packet should not move state ownership outside `LLTexLayerSet`.

## Risk

Risk is medium.

Why:

- the function controls render state directly;
- helper extraction must preserve `LLGLSUIDefault` and `LLGLDepthTest` scope;
- helper extraction must preserve flush placement around each layer render;
- invisible-mask behavior affects whether alpha masks run at all.

## Not Allowed First

Do not change:

- color values;
- alpha minimum values;
- blend modes;
- flush placement;
- layer iteration order;
- `renderAlphaMaskTextures(...)`;
- `LLTexLayer::render(...)`;
- `LLTexLayerTemplate::render(...)`;
- morph mask paths.

## Verification Plan

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
