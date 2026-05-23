# Viewer Tex Layer Buffer Owner Map

Date: 2026-05-23

Branch: `phase5`

## Scope

This note maps `LLViewerTexLayerSetBuffer` before any avatar bake source
cleanup.

No source files are modified by this note.

`LLViewerTexLayerSetBuffer` is the avatar-bake branch of the
`LLViewerDynamicTexture` system. It should not be treated like an ordinary UI
preview texture.

## Inputs Inspected

- `docs/architecture/207-dynamic-texture-update-flow.md`
- `docs/architecture/208-dynamic-texture-user-table.md`
- `docs/architecture/209-dynamic-texture-overrides.md`
- `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`
- `docs/architecture/generated/source_inventory.csv`
- `indra/newview/llviewertexlayer.h`
- `indra/newview/llviewertexlayer.cpp`
- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`

## Inventory Snapshot

Current generated inventory signals:

| file | category | `gGL` | `LLGL` | `LLRender` | `LLRenderTarget` |
|---|---|---:|---:|---:|---:|
| `indra/newview/llviewertexlayer.cpp` | `viewer.misc` | 0 | 0 | 0 | 0 |
| `indra/newview/llviewertexlayer.h` | `viewer.misc` | 0 | 0 | 0 | 0 |
| `indra/llappearance/lltexlayer.cpp` | `render.opengl_touching` | 85 | 20 | 17 | 10 |
| `indra/llappearance/lltexlayer.h` | `render.opengl_touching` | 0 | 2 | 0 | 12 |

Important interpretation:

- the viewer-side class owns update eligibility, dynamic texture integration,
  and avatar/self policy;
- the appearance-side base owns most of the immediate render and projection
  behavior.

Do not judge the risk from `llviewertexlayer.cpp` alone.

## Owner Boundary

`LLViewerTexLayerSetBuffer` owns:

- dynamic texture participation through `ORDER_LAST`;
- local avatar bake update requests;
- low-res versus high-res update bookkeeping;
- readiness checks before rendering;
- forwarding dynamic texture hooks into tex-layer rendering;
- GL texture restore/destroy forwarding;
- debug byte-count bookkeeping.

`LLTexLayerSetBuffer` owns:

- projection push/pop for texture-layer rendering;
- binding and unbinding the alpha-mask shader;
- default UI render state around tex-layer compositing;
- calling `LLTexLayerSet::render(...)`;
- calling `midRenderTexLayerSet(success)`;
- restoring color mask and alpha blend state.

`LLViewerTexLayerSet` owns:

- enabling or disabling updates for the avatar tex-layer set;
- creating the viewer composite buffer;
- immediate composite updates;
- access to the self avatar for local texture readiness.

## Dynamic Texture Flow

Construction:

- `LLViewerDynamicTexture(width, height, 4, ORDER_LAST, false)`;
- comment says `ORDER_LAST` must render after visual hints are created;
- `mNeedsUpdate` starts true;
- `mNumLowresUpdates` starts zero;
- update timer starts immediately.

Dynamic texture overrides:

- `needsRender()` performs strict avatar and texture-readiness checks;
- `preRender(...)` ignores the incoming clear-depth parameter and calls
  `preRenderTexLayerSet()`;
- `render()` delegates to `renderTexLayerSet(mBoundTarget)`;
- `postRender(...)` delegates to `postRenderTexLayerSet(success)`.

The phase 4 dynamic texture driver renders this owner in the bake-target pass,
not the preview-target pass.

## `needsRender()` Contract

`needsRender()` returns false unless:

- the agent avatar is valid;
- `mNeedsUpdate` is true;
- `isReadyToUpdate()` is true;
- the avatar is not appearance-animating;
- the requested baked texture is not an invalid skirt bake case;
- local texture data is available.

High-risk detail:

- skirt bake suppression depends on both `getBakedTE(...)` and
  `isWearingWearableType(WT_SKIRT)`;
- this should not be simplified mechanically.

## Update Readiness Contract

`isReadyToUpdate()` returns true when:

- local texture data is final;
- no low-res update has happened yet;
- or a timeout has elapsed and lower LOD data is available.

The timeout is currently a local constant:

- `TEXTURE_TIMEOUT = 10`

This controls whether a lower-resolution local bake is allowed before final
texture data is ready.

## Render Contract

The render path is split across classes:

1. `LLViewerDynamicTexture::updateBakeDynamicTextures(...)` binds
   `gPipeline.mBakeMap`.
2. `LLViewerDynamicTexture::updateDynamicTexture(...)` sets `mBoundTarget`.
3. `LLViewerTexLayerSetBuffer::preRender(...)` calls:
   - `LLTexLayerSetBuffer::preRenderTexLayerSet()`;
   - `LLViewerDynamicTexture::preRender(false)`.
4. `LLViewerTexLayerSetBuffer::render()` calls:
   - `LLTexLayerSetBuffer::renderTexLayerSet(mBoundTarget)`.
5. `LLTexLayerSetBuffer::renderTexLayerSet(...)`:
   - sets the color mask;
   - binds `gAlphaMaskProgram`;
   - sets minimum alpha to `0.004f`;
   - unbinds the vertex buffer;
   - enters `LLGLSUIDefault`;
   - calls `mTexLayerSet->render(...)`;
   - flushes `gGL`;
   - calls `midRenderTexLayerSet(success)`;
   - unbinds `gAlphaMaskProgram`;
   - unbinds the vertex buffer;
   - restores color mask and alpha blend mode.
6. `LLViewerTexLayerSetBuffer::midRenderTexLayerSet(...)` calls `doUpdate()`
   when another update is still ready.
7. `LLViewerTexLayerSetBuffer::postRender(...)` calls:
   - `LLTexLayerSetBuffer::postRenderTexLayerSet(success)`;
   - `LLViewerDynamicTexture::postRender(success)`.

## State To Preserve

Viewer-side state:

- `mNeedsUpdate`;
- `mNumLowresUpdates`;
- `mNeedsUpdateTimer`;
- `sGLByteCount`;
- dynamic texture registration in `ORDER_LAST`;
- `mBoundTarget`;
- `mGLTexturep` created/created-state behavior.

Appearance-side render state:

- projection push/pop;
- `gAlphaMaskProgram`;
- minimum alpha `0.004f`;
- `LLGLSUIDefault`;
- color mask restore;
- alpha blend mode restore;
- vertex-buffer unbinds;
- `gGL.flush()` before `midRenderTexLayerSet(...)`.

Avatar/self state:

- `gAgentAvatarp`;
- appearance animation state;
- local texture data availability/finality;
- skirt wearable state;
- mesh texture update after a local bake update.

## Risk Notes

High-risk behaviors:

- changing `ORDER_LAST`;
- changing `preRender(false)` depth-clear behavior;
- changing when `midRenderTexLayerSet(success)` runs;
- changing `mNeedsUpdate` or low-res update timing;
- changing `setGLTextureCreated(true)`;
- changing local texture final/available checks;
- changing skirt bake suppression;
- moving projection push/pop across dynamic texture pre/post calls;
- changing alpha-mask shader use or minimum alpha.

Medium-risk behaviors:

- extracting small viewer-side predicate helpers from `needsRender()`;
- extracting `doUpdate()` bookkeeping helpers;
- adding docs around render flow and update readiness.

## First Source Packet Candidate

If source work continues here, the first safe packet should be viewer-side only:

- split `LLViewerTexLayerSetBuffer::needsRender()` into private predicate
  helpers.

Allowed helper boundaries:

- update requested and ready;
- avatar appearance animation guard;
- skirt bake guard;
- local texture data availability guard.

Not allowed in the first packet:

- touching `indra/llappearance/lltexlayer.*`;
- touching `LLTexLayerSetBuffer::renderTexLayerSet(...)`;
- changing `ORDER_LAST`;
- changing update timers;
- changing local texture availability/finality policy;
- changing shader or render state;
- changing dynamic texture `preRender(...)`, `render()`, or `postRender(...)`
  behavior.

## Verification Plan

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

Manual runtime smoke is optional for a pure predicate helper split. If
requested, use appearance/outfit texture refresh paths rather than material
preview or map UI.

## Next Small Tasks

1. Decide whether to split `LLViewerTexLayerSetBuffer::needsRender()` into
   owner-local predicate helpers.
2. Keep `LLTexLayerSetBuffer::renderTexLayerSet(...)` untouched until a
   separate appearance-side render contract exists.
3. Keep map UI and core `llui` rendering separate from avatar bake cleanup.
