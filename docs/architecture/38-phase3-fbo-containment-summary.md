# Phase 3 FBO Containment Summary

This document is the review entry point for the first phase 3 containment
packet.

Branch: `phase3`

Base branch: `phase2`

## Purpose

The packet moves only raw framebuffer bind/status OpenGL calls for
`LLRenderTarget` behind `llglcontainment.*`.

This is not a broad OpenGL wrapper pass. `LLRenderTarget` remains the owner of
render target state, tracker updates, stack behavior, viewport policy, and
debug failure policy.

## Related Files

Source files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Documents:

- `docs/architecture/36-phase3-plan.md`
- `docs/architecture/37-llrendertarget-fbo-containment-task.md`
- `docs/architecture/local-darwin-arm64-build.md`

Generated inventory:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

## Commit Packet

The first phase 3 FBO containment packet is:

- `33c5618af9 docs: start phase3 containment plan`
- `8ca875afa8 docs: define first phase3 fbo containment task`
- `bffb117844 llrender: contain render target fbo calls`
- `582c948ceb docs: record phase3 fbo containment check`

## API Added

Added to `LLGLContainment`:

- `bindReadWriteFramebuffer(U32 framebuffer_name)`
- `getDrawFramebufferStatus()`

These functions wrap:

- `glBindFramebuffer(GL_FRAMEBUFFER, ...)`
- `glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)`

## Ownership Kept Local

The packet keeps these in `LLRenderTarget`:

- `sCurFBO` updates
- `sBoundTarget` stack behavior
- attachment mutation versus render target binding intent
- default framebuffer restore ordering
- viewport restore
- read/draw buffer routing
- `gDebugGL` conditional checks
- warning text and `ll_fail(...)`

## Behavior Expectations

Expected behavior change:

- none

The source patch changes where the raw OpenGL calls are issued, not when they
are issued and not which `LLRenderTarget` state changes around them.

## Verification Performed

Targeted build:

- `llrender/fast`: passed
- rebuilt `llrendertarget.cpp.o`
- rebuilt `llglcontainment.cpp.o`
- relinked `libllrender.a`

Generated inventory:

- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

Integration build:

- local incremental Xcode arm64 Release build: passed
- no `clean` build was run
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` verified in the app bundle

## Review Focus

Reviewers should check:

- `LLGLContainment` API surface is limited to FBO bind/status calls
- `LLRenderTarget` local helper names still express intent
- `sCurFBO` assignments did not move into `LLGLContainment`
- attachment mutation helpers still do not update `sCurFBO`
- `gDebugGL`, warning, and `ll_fail(...)` behavior stayed local
- no other renderer owner was touched
- no public `LLRenderTarget` API changed

## Remaining Risk

Risk level: medium.

This packet proves the first narrow containment move compiles and links, but
it does not prove visual correctness across all render target users. Remaining
risk is mostly existing global OpenGL state risk:

- nested render target stack ordering
- temporary attachment mutation binding
- default framebuffer restore during window-size changes
- render target use from pipeline, dynamic textures, probes, and previews

## Stop Point

Stop here before moving another OpenGL family into `llglcontainment.*`.

Recommended next step:

- review this packet
- launch the viewer for a runtime smoke test if desired
- choose the next phase 3 candidate only after review
