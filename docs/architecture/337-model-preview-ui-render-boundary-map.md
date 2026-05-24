# Model Preview UI/Render Boundary Map

Date: 2026-05-24

Branch: `phase11`

## Scope

This note maps UI mutation still mixed into the model upload preview render
path after phase 10.

No source behavior is changed by this note.

## Files Inspected

- `docs/architecture/335-phase10-completion-summary.md`
- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`

## Current Render-Path UI Coupling

`LLModelPreview::render()` still:

- reads upload UI controls:
  - `upload_skin`
  - `upload_joints`
  - `physics_explode`
- calls `updateSkinPreviewControls(...)`, which mutates floater controls;
- enables `reset_btn` when the current preview LOD has a model.

`LLModelPreview::updateSkinPreviewControls(...)` currently mixes:

- model-state inspection:
  - scan preview scene;
  - set each model pelvis offset;
  - detect skin weights;
  - read legacy rig flags;
  - read joint upload validity;
- model option mutation:
  - update `mViewOption["show_skin_weight"]`;
  - update `mFirstSkinUpdate`;
  - update `mLastJointUpdate`;
- floater UI mutation:
  - `clearAvatarTab()`;
  - `updateAvatarTab(...)`;
  - `childSetValue("upload_skin", ...)`;
  - `childSetValue("upload_joints", ...)`;
  - `childSetValue("show_skin_weight", ...)`;
  - `childEnable("upload_skin")`;
  - `childDisable("upload_skin")`;
  - `childSetVisible("skin_too_many_joints", ...)`;
  - `childSetVisible("skin_unknown_joint", ...)`;
  - `enableViewOption(...)`;
  - `disableViewOption(...)`;
  - `setViewOptionEnabled(...)`;
  - `childEnable("lock_scale_if_joint_position")`;
  - `childDisable("lock_scale_if_joint_position")`;
  - `childSetValue("lock_scale_if_joint_position", ...)`;
  - `childSetEnabled("upload_joints", ...)`.

## Proposed Ownership

`LLModelPreview` should own:

- scene/model scanning;
- pelvis offset assignment;
- skin weight detection;
- render option values used by drawing;
- legacy rig state and joint-upload validity state.

`LLFloaterModelPreview` should own:

- enabling and disabling controls;
- setting control values;
- showing skin warning controls;
- clearing and repopulating the avatar tab;
- keeping upload controls and view option controls synchronized.

## First Boundary

The first source boundary should move skin preview UI control synchronization
to a floater method.

The model preview render path may still call that method in the first packet.
That preserves timing while changing ownership.

Later packets can decide whether the floater should apply pending UI state from
`draw()` or from model load/update callbacks instead of from render.

## Risks

High risk:

- changing when `upload_skin` is auto-enabled for models with weights;
- changing when `show_skin_weight` is auto-enabled;
- changing avatar tab clear/populate order;
- changing joint upload control enablement;
- hiding rig warning controls differently.

Medium risk:

- shifting control updates by one frame;
- changing `mViewOption` synchronization with checkbox values;
- using `LLFloaterModelPreview::sInstance` when the model preview already has
  `mFMP`.

Low risk:

- moving the same UI operations to a floater-owned method while preserving the
  same call site and order;
- documenting the remaining `reset_btn` render-path mutation before moving it.

## First Source Packet Candidate

Add a floater-owned method for skin preview UI synchronization and call it from
the existing render-time preparation point.

Allowed files:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`
- `indra/newview/llfloatermodelpreview.h`
- `indra/newview/llfloatermodelpreview.cpp`

Do not change:

- render order;
- dynamic texture update order;
- model loading;
- LOD generation;
- physics preview;
- skinned draw semantics;
- `needsRender()`.
