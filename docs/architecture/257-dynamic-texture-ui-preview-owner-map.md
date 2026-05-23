# Dynamic Texture UI Preview Owner Map

Date: 2026-05-23

Branch: `phase4`

## Scope

This note opens the next phase 4 owner after the completed
`LLDrawPoolAlpha` cleanup block.

No source files are modified by this note. Its purpose is to name the exact
owner, ordering, state, risk, and verification surface required before any
further source packet in this area.

## Inputs Inspected

- `docs/architecture/206-ui-render-boundaries-post-containment.md`
- `docs/architecture/207-dynamic-texture-update-flow.md`
- `docs/architecture/208-dynamic-texture-user-table.md`
- `docs/architecture/209-dynamic-texture-overrides.md`
- `docs/architecture/210-map-ui-render-boundaries.md`
- `docs/architecture/239-phase4-plan.md`
- `indra/newview/lldynamictexture.h`
- `indra/newview/lldynamictexture.cpp`
- source search for `LLViewerDynamicTexture` subclasses and overrides in
  `indra/newview/`

## Why This Owner Is Next

`LLViewerDynamicTexture` is the narrowest next boundary that still matters for
the long-term UI/render split.

It is not ordinary UI drawing. It is a shared render driver that lets UI and
appearance workflows render content into pipeline-owned render targets and copy
the result into viewer textures.

This makes it a better next phase 4 target than a broad pass over `llui`:

- it has one central owner file;
- it already has a documented update flow;
- it connects UI preview rendering to render targets, viewport state, camera
  state, texture copies, and avatar bake paths;
- it can be improved in owner-local packets without moving files or changing
  runtime behavior.

## Owner Boundary

`LLViewerDynamicTexture` owns:

- static instance registration by order bucket;
- target selection between the preview target and the bake target;
- validation of the borrowed render targets;
- per-instance update order;
- default viewport setup for dynamic texture dimensions;
- default camera save and restore;
- default framebuffer-to-texture copy after successful render;
- final return value for whether any dynamic texture rendered.

It does not own:

- model preview scene rendering;
- image preview avatar or sculpt rendering;
- BVH animation preview rendering;
- GLTF material preview rendering;
- avatar bake composition policy;
- visual parameter mutation policy;
- UI controls that create, show, or destroy preview objects.

## File Groups

### Driver

| file | role | risk |
|---|---|---|
| `indra/newview/lldynamictexture.h` | `LLViewerDynamicTexture` API, order enum, static helpers, instance list | high |
| `indra/newview/lldynamictexture.cpp` | render-target validation, range update, default pre/post render, GL texture restore/destroy | high |

### Preview Users

| user | file | order | target | visible surface | risk |
|---|---|---:|---|---|---|
| `LLModelPreview` | `indra/newview/llmodelpreview.cpp` | `ORDER_MIDDLE` | preview target | model upload preview | high |
| `LLImagePreviewAvatar` | `indra/newview/llfloaterimagepreview.cpp` | `ORDER_MIDDLE` | preview target | image upload avatar preview | medium-high |
| `LLImagePreviewSculpted` | `indra/newview/llfloaterimagepreview.cpp` | `ORDER_MIDDLE` | preview target | sculpt upload preview | medium-high |
| `LLPreviewAnimation` | `indra/newview/llfloaterbvhpreview.h` / `.cpp` | `ORDER_MIDDLE` | preview target | BVH animation upload preview | medium-high |
| `LLGLTFPreviewTexture` | `indra/newview/llgltfmaterialpreviewmgr.cpp` | `ORDER_MIDDLE` | preview target | GLTF material preview | high |

### Avatar And Appearance Users

| user | file | order | target | visible surface | risk |
|---|---|---:|---|---|---|
| `LLViewerTexLayerSetBuffer` | `indra/newview/llviewertexlayer.cpp` | `ORDER_LAST` | bake target | avatar baked texture composition | high |
| `LLVisualParamHint` | `indra/newview/lltoolmorph.cpp` | `ORDER_MIDDLE` | preview target | appearance editor visual parameter hint | high |
| `LLVisualParamReset` | `indra/newview/lltoolmorph.cpp` | `ORDER_RESET` | bake target group | appearance reset path | medium |

### UI Entry Points And Adjacent Boundaries

These files create or display preview users, but should not be edited as part
of a driver-only packet:

- `indra/newview/llfloatermodelpreview.cpp`
- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llfloaterbvhpreview.cpp`
- `indra/newview/lltexturectrl.cpp`

Adjacent UI/render boundaries that are not `LLViewerDynamicTexture` users:

- `indra/newview/llsnapshotlivepreview.cpp`
- `indra/newview/llnetmap.cpp`
- `indra/newview/llworldmapview.cpp`
- core `indra/llui/` draw paths

These should stay separate from dynamic texture cleanup.

## Current Ordering Contract

`LLViewerDynamicTexture::updateAllInstances()` currently preserves this order:

1. Reset `sNumRenders`.
2. Return `true` immediately when GL is disabled.
3. Borrow `gPipeline.mAuxillaryRT.deferredScreen` as the preview target.
4. Borrow `gPipeline.mBakeMap` as the bake target.
5. Validate both borrowed targets before rendering either range.
6. Bind and clear the preview target.
7. Unbind the active shader and vertex buffer.
8. Update `ORDER_FIRST` through the order before `ORDER_LAST`.
9. Flush the preview target.
10. Bind and clear the bake target.
11. Update `ORDER_LAST` through the order before `ORDER_COUNT`.
12. Flush the bake target.
13. Flush `gGL`.
14. Return whether any dynamic texture render returned `true`.

Per-instance update order:

1. Skip the instance when `needsRender()` returns `false`.
2. Assert that the instance fits in the selected target.
3. Clear depth.
4. Set immediate color to white.
5. Store the selected target on the dynamic texture.
6. Call `preRender()`.
7. Call virtual `render()`.
8. Count successful renders.
9. Flush `gGL`.
10. Unbind the vertex buffer.
11. Clear the stored target pointer.
12. Call `postRender(result)`.

This order must not change during a behavior-preserving source packet.

## State To Preserve

State that belongs to the driver:

- `sInstances[ORDER_COUNT]`
- `sNumRenders`
- `mBoundTarget`
- `mOrigin`
- saved `mCamera`
- `mClamp`
- generated dynamic texture GL storage

Borrowed global state:

- `gPipeline.mAuxillaryRT.deferredScreen`
- `gPipeline.mBakeMap`
- `gViewerWindow` 2D viewport restoration
- `LLViewerCamera` current camera
- `gGL` immediate render state
- active shader and vertex buffer bindings

The driver borrows pipeline render targets. It does not allocate or resize
them.

## Risk Notes

High-risk behaviors:

- changing the split between preview target and bake target;
- changing whether both targets are validated before preview rendering starts;
- changing shader or vertex-buffer unbind placement;
- changing the clear/bind/flush order around either target;
- moving `postRender(result)` before `mBoundTarget` is cleared;
- changing camera restore or `gViewerWindow->setup2DViewport()` timing;
- treating `LLViewerTexLayerSetBuffer` like an ordinary UI preview texture;
- "fixing" `LLPreviewAnimation` or `LLVisualParamReset` refresh behavior
  without a dedicated task.

Medium-risk behaviors:

- renaming or splitting owner-local helpers without changing call order;
- adding comments that document target ownership;
- adding docs that separate preview users from avatar bake users.

## Source Packet Candidate

If source work continues here, the next packet should be restricted to
`LLViewerDynamicTexture` and should only split pass-level orchestration inside
`updateAllInstances()`.

Allowed shape:

- keep `validateDynamicTextureTargets(...)` unchanged;
- keep `updateDynamicTexture(...)` unchanged;
- keep `updateDynamicTextureRange(...)` unchanged;
- add owner-local helpers that separate the preview-target pass from the
  bake-target pass;
- keep the exact bind, clear, shader unbind, vertex-buffer unbind, range,
  flush, and final `gGL.flush()` order.

Not allowed in that packet:

- changing subclass virtual methods;
- changing order enum values;
- changing render-target selection;
- changing return-value semantics;
- changing viewport, camera, or texture-copy behavior;
- editing map UI, snapshot preview, or core `llui` draw code.

## Verification Plan For A Source Packet

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Manual smoke is optional for a pure pass-split helper extraction because the
packet must preserve call order and behavior. If runtime validation is
requested, use login plus one preview surface rather than a full scene test.

## Next Small Tasks

1. Decide whether to apply the owner-local preview/bake pass split in
   `LLViewerDynamicTexture`.
2. If applied, record the source summary in a follow-up architecture note.
3. Map `LLViewerTexLayerSetBuffer` separately before any avatar bake cleanup.
4. Map `LLGLTFPreviewTexture` separately before material preview changes.
5. Keep map UI and core `llui` rendering outside this dynamic texture packet.
