# Image Preview Sculpted Map

Date: 2026-05-24

Branch: `phase8`

## Scope

This note maps `LLImagePreviewSculpted` before source cleanup in the image
upload sculpt preview dynamic texture path.

No source files are modified by this note.

Immediate owner:

- `LLImagePreviewSculpted` in `indra/newview/llfloaterimagepreview.*`

## Inputs Inspected

- `docs/architecture/208-dynamic-texture-user-table.md`
- `docs/architecture/209-dynamic-texture-overrides.md`
- `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`
- `docs/architecture/307-phase8-plan.md`
- `docs/architecture/310-image-preview-avatar-render-summary.md`
- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llfloaterimagepreview.cpp`

## Owner Boundary

`LLImagePreviewSculpted` owns:

- dynamic texture registration in `ORDER_MIDDLE`;
- sculpted preview volume creation;
- rebuilding the preview vertex buffer from the uploaded image;
- camera yaw, pitch, zoom, and pan state;
- marking itself dirty through `mNeedsUpdate`;
- drawing the preview background into the dynamic texture;
- applying the preview camera to `LLViewerCamera`;
- rendering the sculpted volume with preview lighting.

It does not own:

- image file loading or scaling;
- upload cost, encoding, permissions, or inventory upload;
- UI combo-box mode selection;
- dynamic texture target selection;
- base dynamic texture viewport/camera restore;
- avatar preview rendering.

## Current Dynamic Texture Flow

Construction:

1. Creates a 3-component `LLViewerDynamicTexture` in `ORDER_MIDDLE`.
2. Initializes camera state and marks `mNeedsUpdate`.
3. Creates a sphere sculpt preview `LLVolume`.

Target selection:

1. Resets camera distance, zoom, pitch, yaw, and offset.
2. If an image exists, sculpts the volume from the image data.
3. Allocates a vertex buffer from the sculpted volume face.
4. Copies positions, normalized normals, texture coordinates, and indices.
5. Unmaps the vertex buffer.

Render:

1. Clears `mNeedsUpdate`.
2. Creates UI/default, no-blend, cull-face, and depth-test scopes.
3. Pushes projection/modelview matrices.
4. Draws the dark preview background.
5. Pops projection/modelview matrices.
6. Clears depth.
7. Computes camera rotation from pitch/yaw.
8. Applies camera origin/look-at, aspect, view, and perspective.
9. Reads the sculpted volume face index count.
10. Enables avatar and preview lighting.
11. Binds `gObjectPreviewProgram`.
12. Pushes model matrix, scales, sets diffuse brightness, binds the vertex
    buffer, and draws the sculpted volume.
13. Pops model matrix.
14. Unbinds `gObjectPreviewProgram`.
15. Returns true.

## State To Preserve

Dirty/update state:

- `needsRender()` continues to return `mNeedsUpdate`;
- `render()` clears `mNeedsUpdate` before drawing;
- `refresh()` sets `mNeedsUpdate`.

Render state:

- `LLGLSUIDefault`, no-blend, cull-face, and depth-test lifetimes remain in
  `render()`;
- projection/modelview push/pop remains paired;
- background color remains `(0.15f, 0.2f, 0.3f, 1.f)`;
- `gUIProgram` binding stays before background draw;
- depth clear remains after background draw and before camera setup;
- preview shader bind/unbind remains around the volume draw;
- scale and brightness constants remain unchanged.

Camera state:

- target position remains `(0, 0, 0)`;
- camera rotation is pitch around Y multiplied by yaw around Z;
- camera origin, look-at, aspect, view, and perspective are unchanged.

Geometry state:

- volume face index count still comes from `mVolume->getVolumeFace(0)`;
- `mVertexBuffer->setBuffer()` remains before draw;
- draw primitive remains `LLRender::TRIANGLES`.

## Risk

Risk is medium.

Why:

- the function touches matrix state, camera state, shader state, lighting, and
  vertex buffer drawing;
- the preview volume data is rebuilt from user-provided image data;
- the dynamic texture base class owns viewport/camera restore after render.

## Safe Source Packet Candidate

Split `LLImagePreviewSculpted::render()` into private owner-local helpers.

Allowed helpers:

- `drawPreviewBackground()`
- `applyPreviewCamera()`
- `renderSculptedVolume()`

Constraints:

- keep GL state scope lifetimes in `render()`;
- keep projection/modelview push/pop order in `render()`;
- do not change vertex buffer build logic;
- do not change camera formulas;
- do not touch `LLImagePreviewAvatar` in the same packet.

## Verification Plan

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterimagepreview.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
