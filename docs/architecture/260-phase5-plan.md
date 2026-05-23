# Phase 5 Plan

Date: 2026-05-23

Branch: `phase5`

Base branch: `phase4`

## Purpose

Phase 5 starts after the phase 4 owner-local cleanup checkpoint.

The goal is to move from broad OpenGL containment and draw-pool cleanup toward
concrete UI/render boundary cleanup. The work should still be incremental:
map one owner, preserve behavior, make small source packets, then verify.

This is not a backend port phase.

## Baseline From Phase 4

Phase 4 ended with:

- `LLDrawPoolAlpha` reorganized into owner-local helpers;
- `LLViewerDynamicTexture` preview and bake target passes split into private
  helpers;
- OpenGL containment guardrails passing;
- targeted builds passing;
- a non-clean `mare-viewer` integration checkpoint passing with
  `[100%] Built target mare-viewer`;
- known unrelated warnings documented.

Phase 5 must preserve this baseline.

## Phase 5 Direction

Primary direction:

- split UI/render ownership around dynamic preview users and adjacent UI
  render bridges.

The first owner is:

- `LLGLTFPreviewTexture` in `indra/newview/llgltfmaterialpreviewmgr.*`

Reason:

- it is a high-signal UI/render bridge;
- it is narrower than avatar bake cleanup;
- it is a `LLViewerDynamicTexture` user, so phase 4 prepared the driver
  boundary already;
- it touches pipeline render targets, shaders, post-processing, material load
  state, UI texture picker entry points, and dynamic texture copy-back;
- it can be mapped before touching source.

## Allowed Work

Allowed phase 5 work:

- architecture notes under `docs/architecture/`;
- updates to `todo.md` and `AGENTS.md`;
- owner-local helper extraction after a task-specific owner map exists;
- behavior-preserving cleanup that separates owner responsibilities;
- guardrail/tooling updates if they make renderer boundary work safer;
- targeted builds for touched owners;
- non-clean integration builds only at meaningful checkpoints.

## Not Allowed

Do not do these in phase 5:

- start a Vulkan, Metal, or SDL port;
- change viewer app lifecycle;
- implement multi-window or multi-login behavior;
- move source files;
- rewrite renderer architecture;
- change material preview visual policy without a dedicated task;
- change GLTF material load policy without a dedicated task;
- change shader selection or post-processing semantics without a dedicated
  task;
- change avatar bake behavior while working on GLTF material preview.

## First Candidate: `LLGLTFPreviewTexture`

Initial work is docs-only:

- map `LLGLTFMaterialPreviewMgr::getPreview(...)`;
- map `LLGLTFPreviewTexture::needsRender()`;
- map `preRender(...)`, `render()`, and `postRender(...)`;
- list pipeline state mutations and restore requirements;
- list UI entry points in `LLTextureCtrl` and `LLFloaterTexturePicker`;
- identify a source packet that preserves behavior.

Only after that map exists, a source packet may be considered.

## Candidate Source Packet

Recommended first source packet:

- split `LLGLTFPreviewTexture::render()` into private/local helper functions
  for:
  - temporary pipeline state setup;
  - preview camera and sphere setup;
  - alpha preview sphere draw;
  - post-processing chain;
  - final copy into the dynamic texture target;
  - cleanup.

Constraints:

- no visual behavior change;
- no shader change;
- no render-target change;
- no material load change;
- no UI entry-point change;
- no dynamic texture lifecycle change.

## Verification

For docs-only packets:

```sh
git diff --check
```

For source packets:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Targeted build for the first owner:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llgltfmaterialpreviewmgr.cpp.o -j8
```

If `LLTextureCtrl` is touched, also build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltexturectrl.cpp.o -j8
```

Run a non-clean `mare-viewer` checkpoint only after a meaningful source
milestone, not after every helper split.

## Exit Criteria

Phase 5 can stop after:

- one UI/render owner is mapped;
- at least one behavior-preserving source packet is validated, if safe;
- guardrails still pass;
- targeted build checks pass;
- a non-clean integration checkpoint is run for a source milestone or
  explicitly deferred with a documented reason.
