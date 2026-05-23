# Phase 6 Plan

Date: 2026-05-23

Branch: `phase6`

Base branch: `phase5`

## Purpose

Phase 6 starts after the completed phase 5 checkpoint.

The goal is to continue separating UI/render ownership around appearance editor
preview surfaces without changing viewer runtime behavior.

This phase stays narrow. It starts with appearance visual-parameter preview
dynamic textures before moving to broader UI rendering or screenshot preview
work.

## Baseline From Phase 5

Phase 5 ended with:

- GLTF material preview mapped and split into owner-local helpers;
- viewer and appearance tex-layer dynamic texture boundaries mapped;
- tex-layer render blocks split into owner-local helpers;
- morph-mask alpha-cache key and eviction policy split without touching
  readback;
- OpenGL containment guardrails passing;
- targeted builds passing;
- final non-clean `mare-viewer` integration checkpoint passing with
  `[100%] Built target mare-viewer`.

Phase 6 must preserve this baseline.

## Phase 6 Direction

Primary direction:

- map and clean up appearance-editor preview owners that still sit on
  `LLViewerDynamicTexture`.

The first owner is:

- `LLVisualParamHint` / `LLVisualParamReset` in
  `indra/newview/lltoolmorph.*`

Reason:

- it is a UI-visible appearance editor preview path;
- it is a `LLViewerDynamicTexture` user in `ORDER_MIDDLE`;
- it mutates avatar visual params temporarily;
- it renders through preview-target/camera/impostor state;
- it is narrower than a broad pass over core `llui` draw paths;
- phase 5 already covered adjacent avatar bake texture compositing.

## Allowed Work

Allowed phase 6 work:

- architecture notes under `docs/architecture/`;
- updates to `todo.md` and `AGENTS.md`;
- owner-local helper extraction after a task-specific map exists;
- behavior-preserving cleanup that names appearance preview ownership;
- targeted builds for touched owners;
- non-clean integration builds only at meaningful checkpoints.

## Not Allowed

Do not do these in phase 6:

- start a Vulkan, Metal, or SDL port;
- change viewer app lifecycle;
- implement multi-window or multi-login behavior;
- move source files;
- rewrite renderer architecture;
- change appearance editor UI behavior;
- change visual parameter values;
- change camera policy for appearance hints;
- change dynamic texture order buckets;
- change morph-mask readback behavior.

## First Candidate: `LLVisualParamHint`

Initial work is docs-first:

- map `LLVisualParamHint::needsRender()`;
- map `LLVisualParamHint::preRender(...)`;
- map `LLVisualParamHint::render()`;
- map `LLVisualParamHint::draw(...)`;
- map `LLVisualParamReset::render()`;
- list avatar state mutation and restore requirements;
- list render-state and camera-state requirements;
- identify one behavior-preserving source packet.

## Candidate Source Packet

Recommended first source packet:

- split `LLVisualParamHint::needsRender()` into owner-local predicate helpers.

Reason:

- it is smaller than touching matrix, camera, impostor, or avatar restore
  ordering;
- it documents the update gate without changing render behavior;
- it prepares later render/preRender cleanup by naming the owner contract first.

Constraints:

- preserve `mDelayFrames-- <= 0` post-decrement behavior exactly;
- preserve animation and update-allowance gates;
- do not change `requestHintUpdates(...)`;
- do not change `preRender(...)`, `render()`, `draw(...)`, or
  `LLVisualParamReset::render()`.

## Verification

For docs-only packets:

```sh
git diff --check
```

For source packets:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Run a non-clean `mare-viewer` checkpoint only after a meaningful phase 6
source milestone.

## Exit Criteria

Phase 6 can stop after:

- `LLVisualParamHint` / `LLVisualParamReset` are mapped;
- at least one behavior-preserving source packet is validated, if safe;
- guardrails still pass;
- targeted build checks pass;
- a non-clean integration checkpoint is run for a source milestone or
  explicitly deferred with a documented reason.
