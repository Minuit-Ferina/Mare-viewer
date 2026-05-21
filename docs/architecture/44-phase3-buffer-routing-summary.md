# Phase 3 Buffer Routing Summary

This document is the review entry point for the phase 3 render target buffer
routing containment packet.

Branch: `phase3`

Base branch: `phase2`

## Purpose

The packet moves only raw draw/read buffer routing calls for `LLRenderTarget`
behind `llglcontainment.*`.

`LLRenderTarget` remains the owner of routing intent, attachment-count
decisions, depth-only behavior, and default framebuffer restore policy.

## Related Files

Source files:

- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
- `indra/llrender/llrendertarget.cpp`

Documents:

- `docs/architecture/17-llrendertarget-buffer-routing-contract.md`
- `docs/architecture/43-llrendertarget-buffer-routing-containment-task.md`
- `docs/architecture/local-darwin-arm64-build.md`

Generated inventory:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

## Commit Packet

The buffer routing containment packet is:

- `d704605a97 docs: define render target buffer routing task`
- `63d27d3fc9 llrender: contain render target buffer routing calls`
- `b0e8945cc9 docs: record render target buffer routing check`

## API Added

Added to `LLGLContainment`:

- `setDrawBuffer(...)`
- `setReadBuffer(...)`
- `setDrawBuffers(...)`

These functions wrap:

- `glDrawBuffer(...)`
- `glReadBuffer(...)`
- `glDrawBuffers(...)`

## Ownership Kept Local

The packet keeps these in `LLRenderTarget`:

- depth-only `GL_NONE` routing
- `GL_COLOR_ATTACHMENT*` array construction
- color attachment count
- render target read buffer selection
- default framebuffer `GL_BACK` restore policy
- ordering relative to FBO binding, viewport restore, and `flush()`

## Behavior Expectations

Expected behavior change:

- none

The source patch changes where the raw OpenGL calls are issued, not which
draw/read buffers are selected or when routing is applied.

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

## Review Result

No blocking issue found in the source diff.

Review points checked:

- `LLGLContainment` gained only draw/read buffer helpers
- depth-only targets still route draw/read buffers to `GL_NONE`
- color targets still route draw buffers to `GL_COLOR_ATTACHMENT0...N`
- render target read buffer still uses `GL_COLOR_ATTACHMENT0`
- default framebuffer restore still uses `GL_BACK`
- routing remains local to render target bind/flush behavior
- no public `LLRenderTarget` API changed
- no `pipeline.cpp`, draw pool, UI, shader, or texture upload code changed

## Remaining Risk

Risk level: medium.

The remaining risk is runtime coverage beyond login. Buffer routing can affect
whether output goes to the expected color attachment or default framebuffer, so
a loaded-scene smoke test is still useful before widening phase 3 beyond
`LLRenderTarget`.

## Stop Point

Stop here before moving another OpenGL family into `llglcontainment.*`.

Recommended next step:

- run a loaded-scene smoke test, or review all phase 3 `LLRenderTarget`
  packets together before choosing another family
