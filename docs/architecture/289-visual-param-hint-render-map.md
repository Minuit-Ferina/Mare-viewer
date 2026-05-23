# Visual Param Hint Render Map

Date: 2026-05-23

Branch: `phase6`

## Scope

This note maps `LLVisualParamHint::render()` before source cleanup in the
appearance editor preview render path.

No source files are modified by this note.

Immediate owner:

- `LLVisualParamHint::render()` in `indra/newview/lltoolmorph.cpp`

## Inputs Inspected

- `docs/architecture/284-visual-param-hint-owner-map.md`
- `docs/architecture/286-visual-param-hint-needs-render-summary.md`
- `docs/architecture/288-visual-param-hint-prerender-summary.md`
- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Owner Boundary

`LLVisualParamHint::render()` owns:

- marking `LLVisualParamReset` dirty;
- UI matrix setup for hint background rendering;
- projection/modelview setup for the dynamic texture dimensions;
- binding `gUIProgram`;
- drawing the hint background;
- marking the hint as updated and visible;
- calculating preview camera target and origin;
- applying the preview camera to `LLViewerCamera`;
- generating the avatar impostor;
- restoring the edited visual param weight on the avatar and wearable;
- clearing wearable volatile state;
- restoring avatar visual params;
- restoring immediate color and marking the GL texture created.

It does not own:

- dynamic texture target selection;
- base dynamic texture framebuffer copy;
- base dynamic texture viewport/camera restore in `postRender(...)`;
- update scheduling;
- pre-render avatar preview weight application.

## Current Render Flow

`render()` currently:

1. Sets `LLVisualParamReset::sDirty` to true.
2. Pushes UI matrix and loads UI identity.
3. Pushes projection matrix and sets ortho projection to full texture size.
4. Pushes modelview matrix and loads identity.
5. Binds `gUIProgram`.
6. Draws the background image under `LLGLSUIDefault`.
7. Pops projection and modelview matrices.
8. Sets `mNeedsUpdate` false and `mIsVisible` true.
9. Reads avatar root joint world rotation when present.
10. Reads target joint world position.
11. Computes camera target from visual-param elevation.
12. Computes camera angle from visual-param camera angle and
    `AppearanceCameraMovement`.
13. Computes camera position from visual-param distance, angle, elevation, and
    avatar rotation.
14. Flushes `gGL`.
15. Sets viewer camera aspect and origin/look-at.
16. Sets viewer camera perspective to the dynamic texture viewport.
17. If the avatar drawable exists:
    - enters depth test;
    - flushes;
    - switches blend to replace;
    - calls `gPipeline.generateImpostor(...)`;
    - restores alpha blending;
    - flushes.
18. Restores the edited visual param weight on avatar and wearable.
19. Clears wearable volatile state when possible.
20. Updates avatar visual params.
21. Restores immediate color to white.
22. Marks the GL texture created.
23. Pops UI matrix.
24. Returns true.

## State To Preserve

Render state:

- UI matrix push/pop pairing;
- projection/modelview push/pop pairing;
- ortho projection dimensions;
- `gUIProgram` binding before background draw;
- `LLGLSUIDefault` scope around background draw;
- `gGL.flush()` before camera setup;
- depth test scope around impostor generation;
- replace blend only around impostor generation;
- alpha blend restore after impostor generation;
- immediate color restore before return.

Camera state:

- avatar root rotation defaults to identity when root joint is missing;
- target joint position comes from `mCamTargetJoint`;
- target offset uses visual-param camera elevation;
- camera angle uses visual-param camera angle and `AppearanceCameraMovement`;
- disabling auto camera adds `F_PI`;
- camera position uses visual-param distance, angle, elevation, and avatar
  rotation;
- camera aspect uses full texture width/height;
- perspective viewport uses `mOrigin`, `mFullWidth`, and `mFullHeight`.

Avatar state:

- visual param weight restore happens after impostor generation;
- wearable weight restore happens after avatar weight restore;
- wearable volatile clear stays after weight restore;
- avatar visual params update after volatile clear.

## Risk

Risk is high.

Why:

- matrix push/pop is manual;
- camera state affects the rendered preview;
- avatar impostor generation uses pipeline render behavior;
- visual parameter restoration must stay after impostor generation;
- base dynamic texture post-render camera restore is outside this function.

## Safe Source Packet Candidate

Do not touch matrix push/pop or impostor generation first.

The safest next source packet is pure camera math extraction:

- `getAvatarRenderRotation() const`
- `getCameraTargetPosition(const LLQuaternion& avatar_rotation) const`
- `getCameraPosition(const LLQuaternion& avatar_rotation,
  const LLVector3& target_joint_pos) const`

Reason:

- these helpers do not touch `gGL`;
- they do not touch visual-param restoration;
- they make the camera contract explicit before render-state cleanup.

## Not Allowed First

Do not change:

- matrix push/pop order;
- `gUIProgram` binding;
- background draw timing;
- `gPipeline.generateImpostor(...)`;
- blend modes;
- visual param restore order;
- wearable volatile policy;
- `mGLTexturep->setGLTextureCreated(true)`.

## Verification Plan

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
