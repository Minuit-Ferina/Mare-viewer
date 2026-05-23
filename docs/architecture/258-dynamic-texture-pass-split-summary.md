# Dynamic Texture Pass Split Summary

Date: 2026-05-23

Branch: `phase4`

## Scope

This source packet closes the `LLViewerDynamicTexture` phase 4 owner map from
`docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`.

The change is intentionally owner-local and behavior-preserving. It separates
the preview-target pass from the bake-target pass inside
`LLViewerDynamicTexture::updateAllInstances()` without changing the render
order, render targets, clears, flushes, shader unbind, vertex-buffer unbind, or
return semantics.

## Files Changed

- `indra/newview/lldynamictexture.h`
- `indra/newview/lldynamictexture.cpp`
- `docs/architecture/generated/source_inventory.csv`

## Source Changes

Added private owner-local helpers:

- `LLViewerDynamicTexture::updatePreviewDynamicTextures(...)`
- `LLViewerDynamicTexture::updateBakeDynamicTextures(...)`

The preview helper preserves:

- binding `gPipeline.mAuxillaryRT.deferredScreen`;
- clearing the preview target;
- unbinding the active shader;
- unbinding the vertex buffer;
- rendering `ORDER_FIRST` through the order before `ORDER_LAST`;
- flushing the preview target.

The bake helper preserves:

- binding `gPipeline.mBakeMap`;
- clearing the bake target;
- rendering `ORDER_LAST` through the order before `ORDER_COUNT`;
- flushing the bake target.

`updateAllInstances()` still:

- resets `sNumRenders`;
- exits early when GL is disabled;
- validates both dynamic texture targets before rendering either pass;
- runs the preview pass before the bake pass;
- flushes `gGL` after both target passes;
- returns the bake pass result, matching the previous assignment behavior.

## Risk

Risk is low-medium.

Why:

- this is a source edit in a high-risk UI/render bridge;
- the edit is only a local orchestration split;
- no subclass virtual behavior changes;
- no render target selection changes;
- no order enum changes;
- no viewport, camera, or texture-copy changes.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o -j8
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

Integration checkpoint:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 mare-viewer -j8
```

Result: passed with `[100%] Built target mare-viewer`.

## Known Warnings During Integration Build

These warnings are pre-existing and match prior phase 4 checkpoints:

- deprecated enum arithmetic in `indra/newview/gltf/llgltfloader.cpp`;
- deprecated enum arithmetic in `indra/newview/llpaneloutfitedit.cpp`;
- `RAND_MAX` integer-to-float conversion warnings in
  `indra/newview/llvoicewebrtc.cpp`;
- linker warnings for platform load commands in prebuilt `libjpeg.a` objects.

No warning was introduced by the `LLViewerDynamicTexture` helper split.

## Runtime Smoke

Manual runtime smoke was not run for this packet.

Reason:

- the source change only extracts the existing preview and bake pass bodies
  into private owner-local helpers;
- the integration build covered the affected owner and dependent `newview`
  target;
- no call order or runtime policy was changed.

## Next Boundary

Do not continue by expanding `LLViewerDynamicTexture` into a generic UI
renderer.

The next phase should choose a new owner explicitly, likely one of:

- avatar bake boundary around `LLViewerTexLayerSetBuffer`;
- GLTF material preview boundary around `LLGLTFPreviewTexture`;
- map UI rendering boundary around `LLNetMap` / `LLWorldMapView`;
- core UI clipping and immediate draw boundary in `indra/llui/`.
