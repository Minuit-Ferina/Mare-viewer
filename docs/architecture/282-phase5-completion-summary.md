# Phase 5 Completion Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

Phase 5 is complete.

The phase stayed within the direction set by
`docs/architecture/260-phase5-plan.md`:

- split UI/render ownership around dynamic preview users and adjacent render
  bridges;
- preserve behavior;
- keep work owner-local and reviewable;
- keep OpenGL containment guardrails passing;
- avoid Vulkan, Metal, SDL, app lifecycle, multi-window, and multi-login work.

## Owners Covered

### GLTF Material Preview

Mapped and cleaned up:

- `LLGLTFMaterialPreviewMgr`
- `LLGLTFPreviewTexture`

Completed source packets:

- split `LLGLTFPreviewTexture::render()` into owner-local helpers;
- grouped GLTF preview render state in an owner-local state object.

### Viewer Tex Layer Buffer

Mapped and cleaned up:

- `LLViewerTexLayerSetBuffer`

Completed source packet:

- split `LLViewerTexLayerSetBuffer::needsRender()` into owner-local predicate
  helpers.

### Appearance Tex Layer Buffer

Mapped and cleaned up:

- `LLTexLayerSetBuffer`

Completed source packet:

- wrapped projection push/pop lifetime in an owner-local RAII scope.

### Tex Layer Set Rendering

Mapped and cleaned up:

- `LLTexLayerSet::render(...)`

Completed source packet:

- split visibility, composite clear, color-layer rendering, and invisible clear
  into private owner-local helpers.

### Tex Layer Rendering

Mapped and cleaned up:

- `LLTexLayer::render(...)`

Completed source packet:

- split local texture, static image, and flat color-fill draw tails into private
  owner-local helpers.

### Morph Mask Boundary

Mapped and partially cleaned up:

- `LLTexLayer::renderMorphMasks(...)`
- adjacent alpha-cache access through `getAlphaData()`

Completed source packets:

- shared the alpha-mask cache key calculation;
- split alpha-cache capacity and eviction policy into private owner-local
  helpers.

The readback-heavy parts of `renderMorphMasks(...)` were intentionally not
changed in phase 5.

## Behavior Preserved

Phase 5 did not intentionally change:

- dynamic texture update order;
- preview target versus bake target policy;
- GLTF preview visual policy;
- avatar bake readiness policy;
- tex layer render ordering;
- alpha-mask minimum values;
- blend modes;
- color-mask policy;
- morph-mask readback behavior;
- Intel versus non-Intel readback branching;
- Nsight readback skip behavior;
- app lifecycle;
- windowing;
- login/logout flow.

## Verification

Final guardrails:

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

Final non-clean integration checkpoint:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 mare-viewer -j8
```

Result:

- passed with `[100%] Built target mare-viewer`.

The checkpoint rebuilt a broad part of `newview`, including adjacent phase 5
surfaces such as:

- `llgltfmaterialpreviewmgr.cpp`;
- `llviewertexlayer.cpp`;
- `lltexturectrl.cpp`;
- `lltoolmorph.cpp`;
- `llsnapshotlivepreview.cpp`.

## Known Warnings During Checkpoint

Warnings observed during the final integration build were outside the phase 5
source changes:

- `indra/newview/gltf/llgltfloader.cpp`: deprecated enum arithmetic between
  `LLModelLoader::eLoadState` and `EModelStatus`;
- `indra/newview/llpaneloutfitedit.cpp`: deprecated enum arithmetic between
  `LLPanelOutfitEdit::e_list_view_item_type` and `LLWearableType::EType`;
- `indra/newview/llvoicewebrtc.cpp`: implicit conversion from `RAND_MAX` to
  `F32`;
- linker warnings for platform load commands in packaged `libjpeg.a` x86_64
  assembly objects.

These warnings were not introduced or modified by phase 5.

## Deferred Work

Good next-phase candidates:

- map UI rendering boundaries outside dynamic texture users;
- map `LLVisualParamHint` / `LLVisualParamReset` as appearance-preview dynamic
  texture owners;
- map `LLSnapshotLivePreview` separately from dynamic texture work;
- map `LLTexLayer::renderMorphMasks(...)` readback ownership before touching
  Intel/non-Intel/Nsight paths;
- decide whether phase 6 should stay on appearance-preview cleanup or move to
  broader UI render boundaries.

Do not start those changes without a new phase plan.
