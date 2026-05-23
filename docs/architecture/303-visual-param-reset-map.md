# Visual Param Reset Map

Date: 2026-05-23

Branch: `phase7`

## Scope

This note maps `LLVisualParamReset::render()` before source cleanup in the
appearance editor preview reset path.

No source files are modified by this note.

Immediate owner:

- `LLVisualParamReset::render()` in `indra/newview/lltoolmorph.cpp`

## Inputs Inspected

- `docs/architecture/284-visual-param-hint-owner-map.md`
- `docs/architecture/298-phase6-completion-summary.md`
- `docs/architecture/299-phase7-plan.md`
- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Owner Boundary

`LLVisualParamReset::render()` owns:

- checking `LLVisualParamReset::sDirty`;
- running the avatar appearance reset work after hint dynamic texture renders;
- updating avatar composites;
- updating avatar visual params;
- updating avatar geometry with the current avatar drawable;
- clearing `sDirty`;
- returning `false` because it does not produce a rendered texture.

It does not own:

- setting `sDirty` from hint renders;
- visual-param preview mutation;
- wearable volatile lifetime;
- dynamic texture target setup;
- drawing hint textures into UI.

## Current Flow

`LLVisualParamReset::render()` currently:

1. If `sDirty` is true:
   - calls `gAgentAvatarp->updateComposites()`;
   - calls `gAgentAvatarp->updateVisualParams()`;
   - calls `gAgentAvatarp->updateGeometry(gAgentAvatarp->mDrawable)`;
   - clears `sDirty`.
2. Returns `false`.

## State To Preserve

Dirty state:

- avatar reset work only runs when `sDirty` is true;
- `sDirty` is cleared after all reset work;
- `render()` always returns `false`.

Avatar state:

- composite update happens before visual param update;
- visual param update happens before geometry update;
- geometry update still receives `gAgentAvatarp->mDrawable`;
- no new drawable null guard is added in this packet.

Dynamic texture ordering:

- `LLVisualParamReset` remains `ORDER_RESET`;
- hint renders still set `sDirty` before this reset owner runs.

## Risk

Risk is low to medium.

Why:

- the function is short;
- the ordering is important because it restores avatar state after hint renders;
- adding defensive behavior such as a drawable null guard could be a runtime
  behavior change and is intentionally out of scope.

## Safe Source Packet Candidate

Split the reset work into one private owner-local helper.

Allowed helper:

- `resetAvatarAppearanceState()`

Constraints:

- keep the `sDirty` gate in `render()`;
- keep `sDirty = false` after reset work;
- keep the update order unchanged;
- keep return value `false`;
- do not change `LLVisualParamHint`.

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
