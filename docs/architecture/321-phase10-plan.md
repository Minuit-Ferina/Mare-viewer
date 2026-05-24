# Phase 10 Plan

Date: 2026-05-24

Branch: `phase10`

Base branch: `phase9`

## Purpose

Phase 10 starts after the completed phase 9 checkpoint.

The goal is to start mapping the model upload preview owner without broad
`llui` or `pipeline` work.

This phase starts with `LLModelPreview` in `indra/newview/llmodelpreview.*`.

## Baseline From Phase 9

Phase 9 ended with:

- BVH animation preview render blocks split into owner-local helpers;
- `LLPreviewAnimation::needsUpdate()` versus `needsRender()` documented as a
  possible behavior task;
- OpenGL containment guardrails passing;
- targeted `llfloaterbvhpreview.cpp.o` build passing;
- final non-clean `mare-viewer` integration checkpoint passing with
  `[100%] Built target mare-viewer`.

Phase 10 must preserve this baseline.

## Candidate Choice

Phase 10 considered:

- `LLGLTFPreviewTexture`
- `LLModelPreview`
- `LLPreviewAnimation::needsRender()`

`LLGLTFPreviewTexture` is not selected as first work because the current tree
already contains the phase 5 owner-local helper split and RAII preview state.

`LLPreviewAnimation::needsRender()` is not selected because adding the override
would change dynamic texture skip behavior.

`LLModelPreview` is selected for mapping because it remains the largest model
upload preview owner and a blocker for later UI/render separation.

## Allowed Work

Allowed phase 10 work:

- architecture notes under `docs/architecture/`;
- updates to `todo.md` and `AGENTS.md`;
- very small owner-local helper extraction in `indra/newview/llmodelpreview.*`
  only after task-specific maps exist;
- targeted `llmodelpreview.cpp.o` builds;
- non-clean integration build only after a meaningful source checkpoint.

## Not Allowed

Do not do these in phase 10:

- change model parsing, upload, LOD generation, physics decomposition, or
  validation behavior;
- change skinned preview behavior;
- change `LLPreviewAnimation::needsRender()`;
- touch `pipeline.cpp`;
- begin broad `llui` cleanup;
- move source files;
- change dynamic texture order buckets;
- start app lifecycle, SDL, Vulkan, Metal, multi-window, or multi-login work.

## First Safe Packet

The first possible source packet should be restricted to the canvas background
draw at the top of `LLModelPreview::render()`.

Reason:

- it is a small 2D preview setup block;
- it is before model, physics, avatar, LOD, upload, and UI mutation paths;
- it can be named without changing render order or behavior.

## Verification

For docs-only packets:

```sh
git diff --check
```

For source packets:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

## Exit Criteria

Phase 10 can stop after:

- `LLModelPreview` is mapped;
- at least one behavior-preserving source packet is validated, if safe;
- guardrails still pass;
- targeted build checks pass;
- a non-clean integration checkpoint is run for a source milestone or
  explicitly deferred with a documented reason.
