# Phase 9 Plan

Date: 2026-05-24

Branch: `phase9`

Base branch: `phase8`

## Purpose

Phase 9 starts after the completed phase 8 checkpoint.

The goal is to continue narrowing discrete preview/render owners before broad
`llui` or `pipeline` work.

This phase starts with the BVH animation preview owner in
`indra/newview/llfloaterbvhpreview.*`.

## Baseline From Phase 8

Phase 8 ended with:

- image upload avatar preview render blocks split into owner-local helpers;
- image upload sculpted preview render blocks split into owner-local helpers;
- OpenGL containment guardrails passing;
- targeted `llfloaterimagepreview.cpp.o` builds passing;
- final non-clean `mare-viewer` integration checkpoint passing with
  `[100%] Built target mare-viewer`.

Phase 9 must preserve this baseline.

## Phase 9 Direction

Primary direction:

- map and clean up the BVH animation preview dynamic texture owner;
- keep source packets owner-local and behavior-preserving;
- avoid BVH parser, upload, animation asset, and UI-control behavior changes.

First owner:

- `LLPreviewAnimation` in `indra/newview/llfloaterbvhpreview.*`

Reason:

- it is a `LLViewerDynamicTexture` user in `ORDER_MIDDLE`;
- it renders a dummy avatar into the preview target;
- it is narrower than `LLModelPreview`;
- it shares the same avatar preview rendering shape as phase 8, but belongs to
  the BVH animation upload path;
- a helper split can name existing background, camera, and avatar draw blocks
  without changing render order.

Deferred owners:

- `LLModelPreview`
- `LLGLTFPreviewTexture`
- `LLViewerTexLayerSetBuffer`

Reason:

- they have broader upload, material, bake, or asset ownership risks than the
  BVH preview owner;
- each needs a dedicated map before source cleanup.

## Allowed Work

Allowed phase 9 work:

- architecture notes under `docs/architecture/`;
- updates to `todo.md` and `AGENTS.md`;
- owner-local helper extraction in `indra/newview/llfloaterbvhpreview.*`
  after task-specific maps exist;
- targeted `llfloaterbvhpreview.cpp.o` builds;
- non-clean integration build only after a meaningful source checkpoint.

## Not Allowed

Do not do these in phase 9:

- edit `LLModelPreview`;
- edit `LLGLTFPreviewTexture`;
- edit `LLViewerTexLayerSetBuffer`;
- touch `pipeline.cpp`;
- begin broad `llui` cleanup;
- move source files;
- change BVH parsing, validation, upload, animation playback policy, or UI
  layout;
- change dynamic texture order buckets;
- change camera policy intentionally;
- start app lifecycle, SDL, Vulkan, Metal, multi-window, or multi-login work.

## Verification

For docs-only packets:

```sh
git diff --check
```

For source packets:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterbvhpreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

## Exit Criteria

Phase 9 can stop after:

- `LLPreviewAnimation` is mapped;
- at least one behavior-preserving source packet is validated, if safe;
- guardrails still pass;
- targeted build checks pass;
- a non-clean integration checkpoint is run for a source milestone or
  explicitly deferred with a documented reason.
