# Phase 8 Plan

Date: 2026-05-24

Branch: `phase8`

Base branch: `phase7`

## Purpose

Phase 8 starts after the completed phase 7 checkpoint.

The goal is to continue with narrow `LLViewerDynamicTexture` preview owners
rather than broad `llui` or `pipeline` work.

This phase starts with image upload previews in `indra/newview/llfloaterimagepreview.*`.

## Baseline From Phase 7

Phase 7 ended with:

- remaining `lltoolmorph` appearance preview draw/reset boundaries split into
  owner-local helpers;
- OpenGL containment guardrails passing;
- targeted `lltoolmorph.cpp.o` builds passing;
- final non-clean `mare-viewer` integration checkpoint passing with
  `[100%] Built target mare-viewer`.

Phase 8 must preserve this baseline.

## Phase 8 Direction

Primary direction:

- map and clean up image upload preview dynamic texture owners;
- keep source packets owner-local and behavior-preserving;
- avoid upload behavior, image decoding, UI layout, and broad renderer changes.

First owner:

- `LLImagePreviewAvatar` in `indra/newview/llfloaterimagepreview.*`

Reason:

- it is a `LLViewerDynamicTexture` user in `ORDER_MIDDLE`;
- it renders a dummy avatar into the preview target;
- it is narrower than `LLModelPreview`;
- it is less semantically ambiguous than `LLPreviewAnimation` refresh behavior;
- it shares the same preview-target path already documented for dynamic
  textures.

Second candidate:

- `LLImagePreviewSculpted` in `indra/newview/llfloaterimagepreview.*`

Reason:

- it sits in the same upload floater;
- it is also a dynamic texture preview user;
- it renders a sculpted volume preview with similar camera/background state.

## Allowed Work

Allowed phase 8 work:

- architecture notes under `docs/architecture/`;
- updates to `todo.md` and `AGENTS.md`;
- owner-local helper extraction in `indra/newview/llfloaterimagepreview.*`
  after task-specific maps exist;
- targeted `llfloaterimagepreview.cpp.o` builds;
- non-clean integration build only after a meaningful source checkpoint.

## Not Allowed

Do not do these in phase 8:

- edit `LLModelPreview`;
- edit `LLPreviewAnimation`;
- touch `pipeline.cpp`;
- begin broad `llui` cleanup;
- move source files;
- change image upload validation, costs, encoding, or permissions;
- change preview UI layout or mouse semantics;
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
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloaterimagepreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

## Exit Criteria

Phase 8 can stop after:

- `LLImagePreviewAvatar` is mapped;
- at least one behavior-preserving source packet is validated, if safe;
- `LLImagePreviewSculpted` is mapped or explicitly deferred;
- guardrails still pass;
- targeted build checks pass;
- a non-clean integration checkpoint is run for a source milestone or
  explicitly deferred with a documented reason.
