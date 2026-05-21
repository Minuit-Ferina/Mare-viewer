# Phase 2 Review Summary

This document closes phase 2 for review.

Branch: `phase2`

Base branch: `phase1-gl-containment`

## Status

Phase 2 is complete enough to stop adding more source-side cleanup on this
branch.

The branch should now be reviewed as a stacked architecture/refactor packet.
Further containment work should start from a new phase 3 branch after a precise
task is chosen.

## What Phase 2 Completed

Phase 2 turned the phase 1 evidence into narrower contracts and small local
cleanup packets:

- refined the generated source inventory so raw `gl*` references and likely
  OpenGL API call expressions are tracked separately
- regenerated the architecture inventory and updated OpenGL debt reports
- documented the local macOS arm64 Xcode workflow and the separate Makefile
  validation path
- fixed local Darwin single-config Makefile shared-library staging
- documented the existing manifest `--arch=x86_64` mismatch as local
  build-system debt
- mapped and cleaned local intent inside `LLRenderTarget`
- mapped and cleaned selected local intent inside `LLImageGL`
- mapped and cleaned local intent inside `LLVertexBuffer`
- recorded review summaries for each completed owner packet

## Source-Side Scope

The source cleanup stayed intentionally narrow.

Completed local `.cpp` helper packets:

- `LLRenderTarget`: framebuffer, viewport, attachment, routing, mipmap, clear,
  status, and allocation-error intent names
- `LLImageGL`: pixel store, readback/copy, scratch PBO, and texture sync intent
  names
- `LLVertexBuffer`: buffer lifecycle, binding, upload, attribute layout, and
  draw intent names

The packets did not introduce public API changes and did not move ownership out
of the local owner files.

## Explicitly Not Done

Phase 2 did not:

- add runtime behavior to `llglcontainment.*`
- create a generic OpenGL wrapper
- move renderer files
- touch `pipeline.cpp` behavior
- rewrite draw pools
- rewrite UI rendering paths
- change shader manager behavior
- enable FSR2 on Darwin
- start a Vulkan backend
- choose a release/universal macOS packaging strategy

These exclusions are intentional. They keep the branch reviewable and preserve
upstream mergeability.

## Verification Performed

Targeted checks:

- `llrender/fast` after each source-side helper packet
- `git diff --check` before commits

Integration checks:

- local Xcode arm64 Release build after the completed `LLRenderTarget` packet
- local Xcode arm64 Release build after the completed `LLImageGL` packet
- local Xcode arm64 Release build after the completed `LLVertexBuffer` packet

Latest source-side integration checkpoint:

- commit: `7d8b7dd659 llrender: name llvertexbuffer draw intents`
- result: `** BUILD SUCCEEDED **`
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` verified in the app bundle

The newest commits after that checkpoint are documentation-only.

## Review Entry Points

Use these files first:

- `docs/architecture/13-phase2-plan.md`
- `docs/architecture/25-llrendertarget-phase2-review-summary.md`
- `docs/architecture/31-llimagegl-phase2-review-summary.md`
- `docs/architecture/34-llvertexbuffer-phase2-review-summary.md`
- `docs/architecture/local-darwin-arm64-build.md`

Then inspect source diffs owner by owner rather than reviewing the full branch
as one large change.

## Remaining Risk

Risk level: medium.

The branch improves local ownership names and documentation, but it does not
prove renderer correctness at runtime. Remaining risks:

- build checks do not replace a full graphics regression pass
- `LLRenderTarget`, `LLImageGL`, and `LLVertexBuffer` still own fragile global
  OpenGL state
- source cleanup is intentionally local, so cross-owner containment remains
  unresolved
- manifest architecture mismatch remains local build-system debt
- Makefile app runtime packaging remains non-reference validation

## Recommended Phase 3 Start

Start phase 3 only after reviewing this branch.

Recommended first phase 3 task:

- create a new branch from `phase2`
- choose one exact OpenGL state family with an owner contract already written
- define the proposed `llglcontainment.*` behavior before editing source
- require targeted build verification and one local Xcode arm64 integration
  checkpoint for any source-side behavior change

Good first candidates are narrow and owner-backed:

- framebuffer binding/status behavior from `LLRenderTarget`
- pixel store or scratch PBO state from `LLImageGL`
- buffer binding tracker assumptions from `LLVertexBuffer`

Do not begin with `pipeline.cpp`, draw pools, UI rendering, or shader manager
behavior.
