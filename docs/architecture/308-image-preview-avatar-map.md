# Image Preview Avatar Map

Date: 2026-05-24

Branch: `phase8`

## Scope

This note maps `LLImagePreviewAvatar` before source cleanup in the image upload
preview dynamic texture path.

No source files are modified by this note.

Immediate owner:

- `LLImagePreviewAvatar` in `indra/newview/llfloaterimagepreview.*`

## Inputs Inspected

- `docs/architecture/208-dynamic-texture-user-table.md`
- `docs/architecture/209-dynamic-texture-overrides.md`
- `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`
- `docs/architecture/306-phase7-completion-summary.md`
- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llfloaterimagepreview.cpp`

## Owner Boundary

`LLImagePreviewAvatar` owns:

- dynamic texture registration in `ORDER_MIDDLE`;
- dummy UI avatar creation and destruction;
- selecting a target joint and mesh for the preview;
- assigning and clearing the temporary test texture on preview meshes;
- camera yaw, pitch, zoom, and pan state;
- marking itself dirty through `mNeedsUpdate`;
- drawing the preview background into the dynamic texture;
- applying the preview camera to `LLViewerCamera`;
- rendering the dummy avatar through `LLDrawPoolAvatar`.

It does not own:

- image file loading or scaling;
- upload cost, encoding, permissions, or inventory upload;
- UI combo-box mode selection;
- dynamic texture target selection;
- base dynamic texture viewport/camera restore;
- broad avatar rendering policy.

## Current Dynamic Texture Flow

Construction:

1. Creates a 3-component `LLViewerDynamicTexture` in `ORDER_MIDDLE`.
2. Initializes camera state and marks `mNeedsUpdate`.
3. Creates a dummy UI avatar through `gObjectList`.
4. Sets special render mode on the dummy avatar.

Target selection:

1. Finds the target joint by name.
2. Clears any previous target mesh test texture.
3. Sets the dummy avatar male visual param based on upload mode.
4. Updates dummy avatar visual params and geometry.
5. Hides the dummy avatar root.
6. Finds the target mesh and applies the test texture.
7. Resets camera distance, zoom, pitch, yaw, and offset.

Render:

1. Clears `mNeedsUpdate`.
2. Pushes UI matrix and loads UI identity.
3. Pushes projection/modelview matrices.
4. Draws the dark preview background under `LLGLSUIDefault`.
5. Pops projection/modelview matrices.
6. Flushes `gGL`.
7. Computes target joint position.
8. Computes camera rotation from pitch/yaw.
9. Combines dummy avatar pelvis rotation with camera rotation.
10. Applies camera origin/look-at, aspect, view, and perspective.
11. Unbinds the vertex buffer.
12. Updates dummy avatar LOD.
13. If the dummy avatar drawable exists:
    - enables depth test;
    - disables blending so alpha zero shows material color;
    - finds face 0 and its avatar draw pool;
    - enables preview lighting;
    - renders the dummy avatar only.
14. Pops UI matrix.
15. Restores immediate color to white.
16. Returns true.

## State To Preserve

Dirty/update state:

- `needsRender()` continues to return `mNeedsUpdate`;
- `render()` clears `mNeedsUpdate` before drawing;
- `refresh()` sets `mNeedsUpdate`.

Render state:

- UI matrix push/pop remains paired;
- projection/modelview push/pop remains paired;
- background color remains `(0.15f, 0.2f, 0.3f, 1.f)`;
- `gUIProgram` binding stays before background draw;
- `LLGLSUIDefault` scope remains active across the existing render body;
- `gGL.flush()` remains before camera setup;
- vertex buffer unbind remains before avatar LOD update;
- depth test and blend disable scopes remain around avatar draw;
- immediate color restores to white before return.

Camera state:

- target position still comes from `mTargetJoint`;
- camera rotation is pitch around Y multiplied by yaw around Z;
- dummy avatar pelvis rotation is multiplied by camera rotation;
- camera origin, look-at, aspect, view, and perspective are unchanged.

Avatar state:

- dummy avatar LOD update remains before drawing;
- drawable and face checks remain unchanged;
- preview lighting remains before `renderAvatars(...)`.

## Risk

Risk is medium-high.

Why:

- the function touches UI matrix state, camera state, avatar render state, and
  draw-pool rendering;
- the dynamic texture base class owns viewport/camera restore after render;
- target mesh and dummy avatar state are controlled by upload UI selections.

## Safe Source Packet Candidate

Split `LLImagePreviewAvatar::render()` into private owner-local helpers.

Allowed helpers:

- `drawPreviewBackground()`
- `applyPreviewCamera(LLVOAvatar* avatarp)`
- `renderPreviewAvatar(LLVOAvatar* avatarp)`

Constraints:

- keep `LLGLSUIDefault` lifetime in `render()`;
- keep projection/modelview push/pop order in `render()`;
- keep UI matrix push/pop in `render()`;
- do not change target mesh setup;
- do not change upload behavior;
- do not touch `LLImagePreviewSculpted` in the same packet.

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
