# Preview Animation Map

Date: 2026-05-24

Branch: `phase9`

## Scope

This note maps `LLPreviewAnimation` before source cleanup in the BVH animation
preview path.

No source behavior is changed by this note.

## Files Inspected

- `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`
- `docs/architecture/314-phase8-completion-summary.md`
- `indra/newview/llfloaterbvhpreview.h`
- `indra/newview/llfloaterbvhpreview.cpp`
- `indra/newview/animationexplorer.h`
- `indra/newview/animationexplorer.cpp`

## Owner

`LLPreviewAnimation` owns:

- dummy avatar creation for animation preview;
- dummy avatar special render mode and initial stand motion;
- initial dummy avatar appearance visibility adjustments;
- preview camera distance, yaw, pitch, zoom, and pan offset;
- preview dynamic texture invalidation through `mNeedsUpdate`;
- preview background drawing;
- preview camera setup;
- preview avatar LOD update and avatar pool draw call.

It does not own:

- BVH file parsing;
- animation upload validation or save behavior;
- floater button/slider state;
- animation explorer UI layout;
- global dynamic texture target selection;
- pipeline render-target ownership.

## Entry Points

Constructors and users:

- `LLFloaterBvhPreview::postBuild()` creates a `LLPreviewAnimation`.
- `AnimationExplorer` can also create a `LLPreviewAnimation`.
- Both UI owners bind the preview texture for display and call
  `requestUpdate()` after user interaction or animation time changes.

Render entry:

- `LLViewerDynamicTexture::updateAllInstances()` drives
  `LLPreviewAnimation::render()` as an `ORDER_MIDDLE` preview texture.

## Current Render Order

`LLPreviewAnimation::render()` currently does this:

1. clear `mNeedsUpdate`;
2. store the dummy avatar pointer locally;
3. push projection matrix and set an orthographic UI projection;
4. push modelview matrix and load identity;
5. bind `gUIProgram`;
6. create `LLGLSUIDefault`;
7. unbind texture unit 0;
8. draw the dark preview background;
9. pop projection and modelview matrices;
10. flush `gGL`;
11. compute target position and camera rotation;
12. set the viewer camera origin/look-at;
13. set FOV, aspect, and perspective;
14. update avatar LOD when the drawable exists;
15. unbind vertex buffers;
16. create a depth-test scope;
17. fetch face 0 and its avatar draw pool;
18. dirty avatar mesh;
19. enable preview lights;
20. render only the preview avatar;
21. reset immediate color to white;
22. return `true`.

This order must not change during a behavior-preserving helper split.

## State To Preserve

Owner state:

- `mNeedsUpdate`
- `mCameraDistance`
- `mCameraYaw`
- `mCameraPitch`
- `mCameraZoom`
- `mCameraOffset`
- `mDummyAvatar`

Borrowed global state:

- `gObjectList`
- `gAgent`
- `gUIProgram`
- `gGL`
- `LLViewerCamera`
- `LLVertexBuffer`
- `gPipeline`

## Risk

Risk is medium-high.

Reasons:

- the preview renders an avatar into the shared preview dynamic texture target;
- the same preview owner is used by the BVH upload floater and animation
  explorer;
- render invalidation uses `requestUpdate()` and an owner-local flag, but the
  class currently exposes `needsUpdate()` rather than the base
  `needsRender()` virtual;
- changing refresh semantics could affect animation preview smoothness or
  unnecessary renders.

The helper split should therefore avoid changing invalidation semantics.

## Source Packet Candidate

Split `LLPreviewAnimation::render()` into private owner-local helpers.

Allowed helper names:

- `drawPreviewBackground()`
- `applyPreviewCamera(LLVOAvatar* avatarp)`
- `renderPreviewAvatar(LLVOAvatar* avatarp)`

Allowed source files:

- `indra/newview/llfloaterbvhpreview.h`
- `indra/newview/llfloaterbvhpreview.cpp`

Not allowed in the same packet:

- changing `needsUpdate()` or adding a `needsRender()` override;
- changing BVH parse/upload/playback behavior;
- changing animation explorer behavior;
- changing camera math;
- changing preview texture dimensions or dynamic texture order;
- touching `LLModelPreview`, `LLGLTFPreviewTexture`, `LLViewerTexLayerSetBuffer`,
  `pipeline.cpp`, or broad `llui`.

## Verification Plan

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterbvhpreview.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Manual smoke is optional for a pure owner-local helper split because the packet
must preserve call order and behavior.
