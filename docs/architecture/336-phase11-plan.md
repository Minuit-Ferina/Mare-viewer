# Phase 11 Plan

Date: 2026-05-24

Branch: `phase11`

Base branch: `phase10`

## Purpose

Phase 11 starts after the completed phase 10 checkpoint.

The goal is to begin a real behavior-preserving UI/render separation for the
model upload preview path. This is not another helper-only phase.

The first owner boundary is:

- model and render state stay in `LLModelPreview`;
- UI control mutation moves toward `LLFloaterModelPreview`.

## Baseline From Phase 10

Phase 10 ended with:

- `LLModelPreview::render()` split into owner-local render helpers;
- no `needsRender()` behavior change;
- OpenGL containment and header guardrails passing;
- targeted `llmodelpreview.cpp.o` checks passing for each packet;
- final non-clean `mare-viewer` integration checkpoint passing with
  `[100%] Built target mare-viewer`.

Phase 11 must preserve this baseline.

## Allowed Work

Allowed phase 11 work:

- architecture notes under `docs/architecture/`;
- updates to `AGENTS.md` and `todo.md`;
- narrow source changes in:
  - `indra/newview/llmodelpreview.h`
  - `indra/newview/llmodelpreview.cpp`
  - `indra/newview/llfloatermodelpreview.h`
  - `indra/newview/llfloatermodelpreview.cpp`
- moving UI control mutation ownership from model preview code to floater code
  when the call order and state ownership are documented first;
- targeted object builds for changed files;
- source inventory and GL guardrail checks.

## Not Allowed

Do not do these in phase 11:

- move source files;
- change model parsing, upload, LOD generation, physics decomposition, or
  validation behavior;
- change dynamic texture ordering or `needsRender()` behavior;
- touch `pipeline.cpp`;
- start broad `llui` cleanup;
- change OpenGL containment APIs unless a missing call family is explicitly
  mapped first;
- start app lifecycle, SDL, Vulkan, Metal, multi-window, or multi-login work.

## First Safe Packet

The first source packet should move the skin preview UI control synchronization
out of `LLModelPreview` and into `LLFloaterModelPreview`.

Reason:

- phase 10 already isolated the current render sub-block;
- the current code explicitly says these UI updates should not live in render;
- `LLFloaterModelPreview` already owns related control callbacks and avatar tab
  helpers;
- behavior can be preserved by keeping the same render-time call order while
  shifting UI mutation ownership.

This first packet does not fully remove render-time UI updates. It changes the
owning class first so that a later packet can move timing out of render with a
smaller risk surface.

## Verification

For docs-only packets:

```sh
git diff --check
```

For source packets touching model preview UI sync:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llfloatermodelpreview.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

## Exit Criteria

Phase 11 can stop after:

- at least one UI mutation family has a clear floater-owned boundary;
- behavior-preserving source packets are validated;
- guardrails still pass;
- a non-clean integration checkpoint is run for a meaningful source milestone
  or explicitly deferred with a documented reason.
