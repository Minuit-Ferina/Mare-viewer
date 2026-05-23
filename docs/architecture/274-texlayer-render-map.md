# Tex Layer Render Map

Date: 2026-05-23

Branch: `phase5`

## Scope

This note maps `LLTexLayer::render(...)` before source cleanup inside the
single-layer appearance compositing owner.

No source files are modified by this note.

The immediate owner is:

- `LLTexLayer::render(...)` in `indra/llappearance/lltexlayer.cpp`

## Inputs Inspected

- `AGENTS.md`
- `docs/architecture/271-texlayer-set-render-map.md`
- `docs/architecture/273-texlayer-set-render-helper-summary.md`
- `docs/architecture/generated/source_inventory.csv`
- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Inventory Snapshot

Current generated inventory signals:

| file | category | lines | `gGL` | `LLGL` | `LLRender` | `LLRenderTarget` |
|---|---|---:|---:|---:|---:|---:|
| `indra/llappearance/lltexlayer.cpp` | `render.opengl_touching` | 1977 | 85 | 20 | 17 | 11 |
| `indra/llappearance/lltexlayer.h` | `render.opengl_touching` | 326 | 0 | 2 | 0 | 13 |

There are no direct runtime `gl*` calls here after phase 3 containment, but
`LLTexLayer::render(...)` still directly controls immediate render state
through `gGL`, `gAlphaMaskProgram`, `LLRender::BT_*`, texture binding, and
texture address modes.

## Owner Boundary

`LLTexLayer::render(...)` owns:

- computing the final layer color;
- substituting dummy-avatar color;
- skipping fully transparent layers;
- rendering morph alpha masks before layer color/texture output;
- setting the layer color;
- switching write-all-channels layers to replace blending;
- drawing the local texture path;
- drawing the static image path;
- drawing a flat color fill when there is no RGB texture;
- restoring blend state after alpha-mask or write-all-channel work;
- reporting partial render failure for missing static images.

It does not own:

- parent `LLTexLayerSet` visibility policy;
- parent composite-buffer clearing;
- parent alpha-mask texture pass;
- morph-mask readback internals;
- alpha-cache lifetime;
- template/wearable dispatch in `LLTexLayerTemplate`.

## Current Render Flow

`LLTexLayer::render(...)` currently:

1. Checks for GL errors.
2. Computes `net_color` with `findNetColor(...)`.
3. Overrides color for dummy avatars.
4. Initializes `success` to true.
5. Returns early if the color alpha is zero.
6. If alpha params exist:
   - renders morph masks with `force_render = true`;
   - marks `alpha_mask_specified`;
   - flushes;
   - sets blend function to destination alpha.
7. Sets the current color to `net_color`.
8. If `mWriteAllChannels`:
   - flushes;
   - sets scene blend to replace.
9. If a non-alpha-only local texture is configured:
   - resolves `mLocalTextureObject`;
   - ignores `IMG_DEFAULT_AVATAR`;
   - logs missing local texture data;
   - optionally lowers alpha minimum for write-all-channel output;
   - binds the texture;
   - clamps address mode;
   - draws textured rectangle;
   - restores address mode and unbinds;
   - restores alpha minimum if needed.
10. If a static image is configured:
    - loads it through `LLTexLayerStaticImageList`;
    - draws it if present;
    - marks `success = false` if missing.
11. If there is no RGB texture and a color was explicitly specified:
    - lowers alpha minimum;
    - unbinds texture unit 0;
    - sets `net_color`;
    - draws flat rectangle;
    - restores alpha minimum.
12. If alpha masks or write-all-channel output changed blend state:
    - flushes;
    - restores scene blend to alpha;
    - checks GL errors.
13. Logs partial render failure when `success` is false.
14. Returns `success`.

## State To Preserve

Render state:

- `stop_glerror()` placement at entry and after blend restore;
- `renderMorphMasks(...)` before the layer color is set;
- `gGL.blendFunc(LLRender::BF_DEST_ALPHA,
  LLRender::BF_ONE_MINUS_DEST_ALPHA)` after morph masks;
- `gGL.color4fv(net_color.mV)` before texture/color draws;
- `mWriteAllChannels` replace-blend setup;
- local texture alpha-minimum lowering and restore;
- local texture address-mode restore;
- texture unit 0 unbinds;
- static image failure reporting;
- flat color-fill alpha-minimum lowering and restore;
- final blend restore when alpha masks or write-all-channel output ran.

Data flow:

- dummy avatar color overrides `findNetColor(...)`;
- zero-alpha layers return success without drawing;
- `success` starts true;
- missing local texture data logs but does not fail the render;
- missing static image marks the render partial;
- flat color fill only runs when there is no RGB local/static texture and a
  color was explicitly specified.

## Safe Source Packet Candidate

The next source packet should leave morph-mask setup inline and split only the
texture/color draw tails into private owner-local helpers:

- `renderLocalTexture(S32 width, S32 height)`;
- `renderStaticImage(S32 width, S32 height)`;
- `shouldRenderColorFill(bool color_specified) const`;
- `renderColorFill(const LLColor4& net_color, S32 width, S32 height)`.

This avoids the morph/readback-heavy code paths while making
`LLTexLayer::render(...)` easier to inspect.

## Risk

Risk is medium.

Why:

- local texture rendering changes alpha test and texture address mode;
- static image rendering is the only path in this function that currently
  marks partial failure;
- color-fill rendering depends on a negative condition over both texture paths.

## Not Allowed First

Do not change:

- morph-mask behavior;
- alpha-cache/readback behavior;
- blend factors;
- alpha minimum values;
- texture address modes;
- local texture fallback policy;
- static image failure policy;
- final blend restore timing.

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
