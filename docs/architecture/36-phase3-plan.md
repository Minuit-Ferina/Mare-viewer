# Phase 3 Plan

Branch: `phase3`

Base branch: `phase2`

## Purpose

Phase 3 is the first phase where `llglcontainment.*` may gain narrowly scoped
behavior.

This phase is still not a renderer rewrite. It is also not a generic OpenGL
wrapper pass.

The goal is to move one well-understood OpenGL state family at a time behind a
small containment function only when the owner contract already explains:

- exact callsites
- current owner state
- required ordering
- cleanup or restore behavior
- platform constraints
- verification plan

## Branch Policy

Keep the stack reviewable:

- `phase1-gl-containment`: phase 1 evidence branch
- `phase2`: contracts and local intent cleanup branch
- `phase3`: first behavior-bearing containment branch

Do not merge these branches into local `main` unless that is explicitly chosen
as the distribution strategy.

## Allowed Work

Allowed phase 3 work:

- refine phase 3 contracts under `docs/architecture/`
- add narrow behavior to `indra/llrender/llglcontainment.*`
- update one local owner file per task when that owner already has a phase 2
  contract
- preserve public APIs unless a task explicitly justifies a public boundary
- verify each source packet with a targeted build before moving on

## Not Allowed

Do not do these in phase 3:

- broad replacement of direct `gl*` calls
- direct edits to `pipeline.cpp` behavior as a first task
- draw-pool rewrites
- UI rendering rewrites
- shader manager rewrites
- Vulkan backend work
- FSR2 enablement on Darwin
- renderer file moves
- release packaging, signing, notarization, or DMG work

## First Candidate

Recommended first candidate:

- `LLRenderTarget` framebuffer binding/status containment

Reason:

- `LLRenderTarget` already has phase 2 contracts for FBO binding and
  framebuffer status behavior
- the relevant source-side intent has already been named locally
- the ownership boundary is small compared with texture upload or vertex buffer
  mapping
- the verification path is known: `llrender/fast`, then local Xcode arm64
  Release integration build if source behavior moves into `llglcontainment.*`

Relevant documents:

- `docs/architecture/16-llrendertarget-fbo-contract.md`
- `docs/architecture/22-llrendertarget-framebuffer-status-contract.md`
- `docs/architecture/24-llrendertarget-local-intent-map.md`
- `docs/architecture/25-llrendertarget-phase2-review-summary.md`

## First Task Shape

Before source edits, produce a task-specific containment note that answers:

- which local helper can move or delegate to `llglcontainment.*`
- whether the helper owns only the raw OpenGL call or also tracker updates
- which `LLRenderTarget` static state remains local
- how default framebuffer behavior is preserved
- how `gDebugGL` status checks are preserved
- how `STOP_GLERROR` and `glGetError()` consumption points stay unchanged

Only after that note exists should a source patch be made.

## Verification

For docs-only changes:

- run `git diff --check`

For source changes in `indra/llrender/`:

- build `llrender/fast`
- inspect the diff for public API changes
- run `git diff --check`

For behavior-bearing containment changes:

- run the local incremental Xcode arm64 Release build
- verify the executable is arm64
- verify runtime dylibs are present in the app bundle
- launch and smoke-test the viewer when the touched behavior can affect
  visible rendering

## Exit Criteria

Phase 3 should stop after one small containment family is proven reviewable.

A phase 3 packet is reviewable when:

- the pre-source containment note exists
- the `llglcontainment.*` API surface is narrow
- only one owner file delegates to it
- the owner still owns its local state
- targeted and integration build checks are recorded
- the next candidate is documented but not started in the same commit packet
