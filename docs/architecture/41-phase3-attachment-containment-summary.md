# Phase 3 Attachment Containment Summary

This document is the review entry point for the phase 3 framebuffer texture
attachment containment packet.

Branch: `phase3`

Base branch: `phase2`

## Purpose

The packet moves only the raw framebuffer texture attachment OpenGL call for
`LLRenderTarget` behind `llglcontainment.*`.

`LLRenderTarget` remains the owner of attachment intent, texture ownership,
usage conversion, detach policy, and FBO bind/restore ordering.

## Related Files

Source files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Documents:

- `docs/architecture/18-llrendertarget-attachment-contract.md`
- `docs/architecture/40-llrendertarget-attachment-containment-task.md`
- `docs/architecture/local-darwin-arm64-build.md`

Generated inventory:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

## Commit Packet

The attachment containment packet is:

- `2f0f67ea25 docs: define render target attachment containment task`
- `fea3663f75 llrender: contain render target attachment calls`
- `beaafbbd5b docs: record render target attachment check`

## API Added

Added to `LLGLContainment`:

- `setReadWriteFramebufferTexture2D(...)`

This function wraps:

- `glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, texture_target,
  texture_name, mip_level)`

## Ownership Kept Local

The packet keeps these in `LLRenderTarget`:

- depth versus color attachment selection
- color attachment index calculation
- `LLTexUnit::getInternalType(...)` usage conversion
- mip level `0` selection
- texture name `0` detach convention
- texture deletion order
- depth sharing policy
- framebuffer status check timing
- FBO bind/restore ordering

## Behavior Expectations

Expected behavior change:

- none

The source patch changes where the raw OpenGL call is issued, not which
texture is attached, detached, deleted, or checked.

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

Runtime smoke:

- Xcode-built app launched to the login screen successfully
- no immediate launch or `dyld` failure was reported
- loaded-scene rendering was not covered by this check

## Review Result

No blocking issue found in the source diff.

Review points checked:

- `LLGLContainment` gained only one attachment helper
- `set_framebuffer_texture_attachment(...)` remains the local intent helper
- `clear_framebuffer_texture_attachment(...)` still uses texture name `0`
- `LLTexUnit::getInternalType(usage)` remains local
- mip level `0` remains local
- texture deletion and depth sharing behavior are untouched
- no public `LLRenderTarget` API changed
- no `pipeline.cpp`, draw pool, UI, shader, or texture upload code changed

## Remaining Risk

Risk level: medium.

The remaining risk is runtime coverage beyond login. Attachment behavior
affects FBO completeness and render target consumers, so a loaded-scene smoke
test is still useful before widening phase 3 beyond `LLRenderTarget`.

Watch these areas in runtime testing:

- login screen
- window resize
- default framebuffer restore
- dynamic textures
- reflection probes
- preview widgets

## Stop Point

Stop here before moving another OpenGL family into `llglcontainment.*`.

Recommended next step:

- launch the Xcode-built app for a runtime smoke test, or review the two phase
  3 `LLRenderTarget` packets together before choosing the next candidate
