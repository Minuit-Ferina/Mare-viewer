# Visual Param Hint Owner Map

Date: 2026-05-23

Branch: `phase6`

## Scope

This note maps `LLVisualParamHint` and `LLVisualParamReset` before source
cleanup in the appearance editor preview dynamic texture path.

No source files are modified by this note.

Immediate owners:

- `LLVisualParamHint` in `indra/newview/lltoolmorph.cpp`
- `LLVisualParamReset` in `indra/newview/lltoolmorph.cpp`

## Inputs Inspected

- `docs/architecture/207-dynamic-texture-update-flow.md`
- `docs/architecture/208-dynamic-texture-user-table.md`
- `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`
- `docs/architecture/282-phase5-completion-summary.md`
- `docs/architecture/generated/source_inventory.csv`
- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`
- source references to `LLVisualParamHint` and `LLVisualParamReset`

## Inventory Snapshot

Current generated inventory signals:

| file | category | lines | `gGL` | `LLGL` | `LLRender` | `LLViewerDynamicTexture` |
|---|---|---:|---:|---:|---:|---:|
| `indra/newview/lltoolmorph.cpp` | `render.opengl_touching` | 337 | 37 | 4 | 7 | 28 |
| `indra/newview/lltoolmorph.h` | `viewer.misc` | 118 | 0 | 0 | 0 | 10 |

There are no direct runtime `gl*` calls here after phase 3 containment, but
the file directly controls render state through `gGL`, `LLGLSUIDefault`,
`LLGLDepthTest`, `gUIProgram`, `LLViewerCamera`, and `gPipeline`.

## Owner Boundary

`LLVisualParamHint` owns:

- dynamic texture registration in `ORDER_MIDDLE`;
- hint instance registration in `sInstances`;
- delayed update scheduling;
- update gating for appearance animation and local update allowance;
- temporary wearable/avatar visual parameter mutation;
- blink suppression for preview renders;
- avatar composite and geometry refresh before preview rendering;
- UI projection setup for drawing the hint background;
- preview camera setup around the target joint;
- avatar impostor generation into the dynamic texture target;
- restoration of the edited visual parameter and wearable volatile state;
- drawing the generated hint texture into UI.

`LLVisualParamReset` owns:

- dynamic texture registration in `ORDER_RESET`;
- a dirty flag set by hint rendering;
- restoring avatar composites, visual params, and geometry at the end of the
  dynamic texture update cycle;
- returning false because it does not produce a texture render.

They do not own:

- dynamic texture pass orchestration;
- preview render-target selection;
- base dynamic texture viewport/camera save/restore;
- appearance panel UI creation of hint widgets;
- avatar bake tex-layer compositing.

## Current Flow

### `requestHintUpdates(...)`

Current behavior:

1. Iterates all hint instances.
2. Skips up to two exception hints.
3. If the hint allows updates:
   - sets `mNeedsUpdate`;
   - assigns staggered `mDelayFrames`;
   - increments the stagger for the next allowed hint.
4. If updates are blocked:
   - still sets `mNeedsUpdate`;
   - sets delay to zero.

### `needsRender()`

Current gate:

```cpp
mNeedsUpdate && mDelayFrames-- <= 0 &&
!gAgentAvatarp->getIsAppearanceAnimating() &&
mAllowsUpdates
```

The post-decrement on `mDelayFrames` is part of the behavior.

### `preRender(...)`

Current behavior:

1. Marks the edited wearable volatile when possible.
2. Stores the current visual param weight.
3. Applies the preview weight to the wearable and agent avatar.
4. Forces blink params to zero.
5. Updates composites.
6. Updates visual params through `LLCharacter`.
7. Updates avatar geometry and LOD when the drawable exists.
8. Delegates to `LLViewerDynamicTexture::preRender(...)`.

### `render()`

Current behavior:

1. Sets `LLVisualParamReset::sDirty`.
2. Pushes UI matrix and loads UI identity.
3. Pushes projection/modelview matrices and sets ortho projection.
4. Binds `gUIProgram`.
5. Draws the hint background image under `LLGLSUIDefault`.
6. Pops projection/modelview matrices.
7. Marks update complete and visible.
8. Computes preview camera target and origin from the target joint, avatar
   rotation, visual param camera metadata, and `AppearanceCameraMovement`.
9. Flushes.
10. Sets camera aspect, origin/look-at, and perspective viewport.
11. Generates the avatar impostor under depth test and replace blending when
    the avatar drawable exists.
12. Restores the edited visual param and wearable weight.
13. Clears wearable volatile state when possible.
14. Updates avatar visual params.
15. Restores immediate color to white.
16. Marks the GL texture created.
17. Pops the UI matrix.
18. Returns true.

### `draw(...)`

Current behavior:

1. Returns immediately when the hint is not visible.
2. Binds the dynamic texture.
3. Sets draw color with caller alpha.
4. Draws two triangles under `LLGLSUIDefault`.
5. Unbinds texture unit 0.

### `LLVisualParamReset::render()`

Current behavior:

1. If dirty:
   - updates composites;
   - updates visual params;
   - updates avatar geometry;
   - clears dirty.
2. Returns false.

## State To Preserve

Update scheduling:

- exception hints remain skipped;
- delayed updates remain staggered only for hints that allow updates;
- blocked hints still record `mNeedsUpdate` and delay zero;
- `mDelayFrames-- <= 0` remains post-decrement.

Avatar state:

- wearable volatile state is set before preview mutation and cleared after
  render;
- preview visual param weight is applied to both wearable and avatar;
- previous visual param weight is restored after impostor generation;
- blink params are forced to zero for preview rendering;
- avatar visual params and geometry update order remains unchanged.

Render state:

- UI matrix push/pop remains paired;
- projection/modelview push/pop remains paired;
- `gUIProgram` binding stays before background draw;
- `LLGLSUIDefault` scope still covers background draw;
- camera setup stays before impostor generation;
- `LLGLDepthTest(GL_TRUE, GL_TRUE)` still covers impostor generation;
- blend type switches to replace only around impostor generation and restores
  alpha blending afterward;
- immediate color restores to white before returning.

## Risk

Risk is medium-high.

Why:

- this owner mutates avatar appearance state for preview rendering;
- it depends on dynamic texture camera/viewport restore;
- `needsRender()` contains a side-effecting delay decrement;
- render-state cleanup is manual and matrix push/pop order is important;
- `LLVisualParamReset` relies on the hint render dirty flag and dynamic texture
  order.

## Safe Source Packet Candidate

Recommended first source packet:

- split `LLVisualParamHint::needsRender()` into private predicate helpers.

Allowed helpers:

- `hasPendingUpdate() const`;
- `isUpdateDelayElapsed()`;
- `isAppearanceAnimationBlocked() const`;
- `canRenderHint() const`.

This packet must preserve the exact short-circuit order and post-decrement
behavior.

## Not Allowed First

Do not change:

- `preRender(...)`;
- `render()`;
- `draw(...)`;
- `LLVisualParamReset::render()`;
- camera placement;
- blend modes;
- matrix push/pop behavior;
- visual param values;
- wearable volatile policy.

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
