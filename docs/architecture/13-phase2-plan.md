# Phase 2 Plan

Branch: `phase2`

Base branch: `phase1-gl-containment`

## Purpose

Phase 2 starts from the completed phase 1 evidence branch. Its purpose is to
turn the inventory, maps, and containment notes into very small follow-up tasks.

This phase is still not a renderer rewrite.

## Branch Policy

The local repository is a clone of an upstream project that is not owned here.
Keep the work reviewable as stacked branches:

- `phase1-gl-containment` remains the phase 1 evidence branch.
- `phase2` is a follow-up branch on top of phase 1.
- Future export can be either a complete stack from upstream or a phase 2-only
  patch from `phase1-gl-containment..phase2`.

Do not merge these branches into local `main` unless that is explicitly chosen
as the distribution strategy.

## Allowed Early Work

Preferred early phase 2 work:

- improve architecture tooling under `tools/architecture/`
- regenerate architecture inventory under `docs/architecture/generated/`
- add or refine architecture documents under `docs/architecture/`
- keep `todo.md` and `AGENTS.md` aligned with the current phase
- define precise containment contracts before touching renderer behavior

Source behavior changes should wait until a task names the exact callsite
family, state ownership, ordering assumptions, and verification plan.

## Not Allowed Yet

Do not start with:

- broad OpenGL wrapper patches
- global replacement of direct `gl*` calls
- renderer file moves
- draw-pool rewrites
- `pipeline.cpp` behavior changes
- UI rendering rewrites
- Vulkan backend work
- enabling FSR2 on Darwin

## First Recommended Task

Improve `tools/architecture/source_inventory.py` so generated inventory can
separate:

- raw `gl*` text references
- likely `gl*(...)` API call expressions
- known false positives where possible

Then regenerate:

- `docs/architecture/generated/source_inventory.csv`
- dependent architecture summaries, if their numbers change

Reason: phase 1 found that raw `gl*` references and likely OpenGL API calls are
not the same signal. Splitting them makes later containment tasks easier to
review and reduces the chance of mechanical wrapper changes.

## Verification

For docs and tooling-only changes:

- run the inventory generator
- inspect generated CSV deltas
- run `git diff --check`

For source-side containment changes later:

- build the smallest affected target first
- run the known local macOS arm64 Release build when the source change is not
  trivially compile-only
- compare against the runtime FPS baseline when behavior can affect rendering

## Exit Criteria

Phase 2 can move from tooling/contracts into source containment only when there
is at least one narrow task with:

- exact callsites or callsite family
- intent category
- current owner of the GL state or resource
- expected call ordering
- cleanup or restore requirements
- platform constraints, especially Darwin OpenGL 4.1
- build and runtime verification plan
