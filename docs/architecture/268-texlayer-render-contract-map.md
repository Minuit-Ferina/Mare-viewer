# Tex Layer Render Contract Map

Date: 2026-05-23

Branch: `phase5`

## Scope

This note maps the appearance-side render contract behind
`LLViewerTexLayerSetBuffer`.

No source files are modified by this note.

The immediate owner is:

- `LLTexLayerSetBuffer::renderTexLayerSet(...)` in
  `indra/llappearance/lltexlayer.cpp`

This is separate from the viewer-side update predicate cleanup in
`LLViewerTexLayerSetBuffer`.

## Inputs Inspected

- `docs/architecture/265-viewer-texlayer-buffer-owner-map.md`
- `docs/architecture/267-viewer-texlayer-needs-render-summary.md`
- `docs/architecture/generated/source_inventory.csv`
- `indra/llappearance/lltexlayer.h`
- `indra/llappearance/lltexlayer.cpp`
- `indra/newview/llviewertexlayer.h`
- `indra/newview/llviewertexlayer.cpp`

## Inventory Snapshot

Current generated inventory signals:

| file | category | `gGL` | `LLGL` | `LLRender` | `LLRenderTarget` |
|---|---|---:|---:|---:|---:|
| `indra/llappearance/lltexlayer.cpp` | `render.opengl_touching` | 85 | 20 | 17 | 10 |
| `indra/llappearance/lltexlayer.h` | `render.opengl_touching` | 0 | 2 | 0 | 12 |

There are no direct runtime `gl*` calls here after phase 3 containment, but
this file still owns a large amount of immediate render state through `gGL`,
shader state, texture-layer compositing, and render-target-aware drawing.

## Owner Boundary

`LLTexLayerSetBuffer` owns:

- orthographic projection setup for a composite texture;
- projection restore;
- top-level alpha-mask shader setup for compositing;
- top-level color mask and blend restore;
- top-level vertex-buffer unbinds;
- calling `LLTexLayerSet::render(...)`;
- calling `midRenderTexLayerSet(success)` after the tex-layer set render.

It does not own:

- viewer-side update readiness;
- dynamic texture target selection;
- avatar bake request policy;
- low-res versus final texture readiness policy;
- individual layer render behavior;
- individual alpha/color parameter render behavior.

## Projection Contract

`preRenderTexLayerSet()`:

- calls `pushProjection()`;
- sets projection matrix mode;
- pushes projection;
- loads identity;
- sets `gGL.ortho(...)` to the composite width and height;
- switches to modelview;
- pushes modelview;
- loads identity.

`postRenderTexLayerSet(success)`:

- calls `popProjection()`;
- pops projection;
- switches to modelview;
- pops modelview.

Risk:

- high if moved relative to `LLViewerDynamicTexture::preRender(false)` or
  `LLViewerDynamicTexture::postRender(success)`;
- medium if only wrapped in an owner-local RAII object with the exact same
  push/pop ordering.

## Render Flow

`renderTexLayerSet(bound_target)` currently:

1. Sets color mask to color and alpha enabled.
2. Initializes `success` to true.
3. Binds `gAlphaMaskProgram`.
4. Sets alpha minimum to `0.004f`.
5. Unbinds the current vertex buffer.
6. Enters `LLGLSUIDefault`.
7. Calls `mTexLayerSet->render(...)` with:
   - composite origin X/Y;
   - composite width/height;
   - `bound_target`.
8. Flushes `gGL`.
9. Calls `midRenderTexLayerSet(success)`.
10. Unbinds `gAlphaMaskProgram`.
11. Unbinds the current vertex buffer again.
12. Restores color mask to color and alpha enabled.
13. Restores scene blend type to `LLRender::BT_ALPHA`.
14. Returns `success`.

## State To Preserve

Render state:

- color mask enabled before and after render;
- `gAlphaMaskProgram` binding and unbinding;
- minimum alpha `0.004f`;
- `LLGLSUIDefault` scope around the tex-layer set render;
- vertex-buffer unbind before and after render;
- `gGL.flush()` before `midRenderTexLayerSet(success)`;
- final blend type `LLRender::BT_ALPHA`.

Data flow:

- `bound_target` must be passed through to `LLTexLayerSet::render(...)`;
- `success` must include the tex-layer set render result;
- `midRenderTexLayerSet(success)` must see the same success value;
- `success` must be returned unchanged after cleanup.

## Adjacent Render Owners

This map does not yet cover the full body of:

- `LLTexLayerSet::render(...)`;
- `LLTexLayer::render(...)`;
- `LLTexLayerTemplate::render(...)`;
- alpha/color param render paths.

Those areas contain many more `gGL`, `gAlphaMaskProgram`, blend, and color mask
changes. They need separate maps before source cleanup.

## First Source Packet Candidate

If source work continues here, the first safe packet should be narrow:

- extract an owner-local RAII projection scope for `pushProjection()` /
  `popProjection()`;
- or extract an owner-local render-state scope inside
  `renderTexLayerSet(...)`.

Recommended first source packet:

- projection scope only.

Reason:

- it is smaller than touching shader, color mask, blend, or
  `midRenderTexLayerSet(success)` ordering;
- it names an existing push/pop pair without changing render policy.

## Not Allowed First

Do not change:

- `LLTexLayerSet::render(...)`;
- `LLTexLayer::render(...)`;
- alpha-mask shader selection;
- minimum alpha values;
- color mask policy;
- blend policy;
- `gGL.flush()` placement;
- `midRenderTexLayerSet(success)` placement;
- dynamic texture `preRender(...)` or `postRender(...)`.

## Verification Plan

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f indra/llappearance/CMakeFiles/llappearance.dir/build.make -B indra/llappearance/CMakeFiles/llappearance.dir/lltexlayer.cpp.o -j8
```

If the direct object path is not available in the current Makefile tree, use
the viewer object target that rebuilds `llappearance` as part of the normal
dependency chain.

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

## Next Small Tasks

1. Confirm the exact targeted build path for `llappearance` in the existing
   Makefile tree.
2. Decide whether to add a local projection scope in
   `LLTexLayerSetBuffer`.
3. Do not continue deeper into `LLTexLayerSet::render(...)` until that function
   has its own owner map.
