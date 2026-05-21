# LLVertexBuffer Phase 2 Review Summary

This document is the review entry point for the phase 2 `LLVertexBuffer`
intent-naming packet.

Related source files:

- `indra/llrender/llvertexbuffer.cpp`

Related documents:

- `docs/architecture/33-llvertexbuffer-buffer-binding-update-contract.md`
- `docs/architecture/local-darwin-arm64-build.md`

## Purpose

The packet makes existing OpenGL intent inside `LLVertexBuffer` easier to
review without changing runtime behavior.

It does this by:

- documenting the touched buffer, attribute, and draw call families before or
  alongside source cleanup
- replacing selected direct callsites with local `.cpp` helpers whose names
  describe existing local intent
- keeping ownership inside `LLVertexBuffer`
- avoiding public API changes
- avoiding `llglcontainment.*` until there is a phase 3 cross-owner contract

## Source Change Scope

Touched source:

- `indra/llrender/llvertexbuffer.cpp`

Not touched:

- `indra/llrender/llvertexbuffer.h`
- `indra/llrender/llglcontainment.*`
- `indra/newview/`
- draw pools
- UI rendering paths
- shader managers

## Commit Packet

The `LLVertexBuffer` packet is:

- `c4a3a0ea46 docs: document llvertexbuffer buffer contract`
- `1622ef317b llrender: name llvertexbuffer lifecycle intents`
- `e89e1de605 llrender: name llvertexbuffer binding intents`
- `f06a6bd41d llrender: name llvertexbuffer upload intents`
- `8aa4292cc0 llrender: name llvertexbuffer attribute intents`
- `7d8b7dd659 llrender: name llvertexbuffer draw intents`
- `eb8c030389 docs: record llvertexbuffer xcode integration check`

## Intent Families Covered

The packet covers these existing `LLVertexBuffer` OpenGL families:

- buffer name generation and deletion
- delayed buffer deletion flushing
- buffer target binding
- buffer storage allocation
- buffer sub-data upload
- vertex attribute array enable and disable
- vertex attribute pointer setup
- indexed and non-indexed draw calls

The packet intentionally does not cover:

- static immediate-mode fallback helpers that route through `gGL`
- broader shader attribute ownership outside the local setup path
- draw pool ownership in `indra/newview`
- any generic containment behavior in `llglcontainment.*`

## Behavior Expectations

Expected behavior change:

- none

The patch should preserve:

- VBO name pooling and delayed deletion timing
- the one-buffer-at-a-time AMD workaround
- Apple versus non-Apple allocation and unmap behavior
- `sGLRenderBuffer` and `sGLRenderIndices` tracker update placement
- dirty-region merging and upload block splitting
- `GL_DYNAMIC_DRAW` and `GL_STATIC_DRAW` usage
- shader reserved attribute enum assumptions
- attribute offsets, sizes, types, and normalized flags
- `sLastMask` update placement
- draw validation, matrix sync, and `STOP_GLERROR` placement
- `drawRangeFast(...)` validation and matrix sync differences

## Why Helpers Stay Local

The helpers intentionally stay in `llvertexbuffer.cpp`.

Reason:

- buffer binding depends on `LLVertexBuffer` static trackers
- mapped dirty-region flushing depends on `LLVertexBuffer` mapped memory state
- attribute layout depends on local enum ordering and shader masks
- VBO pooling has local platform and driver policy
- draw call validation depends on local index stride and fast-path rules
- moving these helpers to `llglcontainment.*` now would create a generic
  OpenGL wrapper before a reusable containment contract exists

`llglcontainment.*` remains reserved for a later phase where a cross-owner
contract is explicit.

## Verification Performed

Targeted builds:

- `llrender/fast` after each source-side packet

Observed targeted build result:

- `llvertexbuffer.cpp.o` rebuilt
- `libllrender.a` relinked
- no clean viewer build was run for the small source packets

Integration build:

- local Xcode arm64 Release build
- incremental, no `clean`
- command and result recorded in
  `docs/architecture/local-darwin-arm64-build.md`

Observed integration result:

- `** BUILD SUCCEEDED **`
- `llvertexbuffer.cpp.o` rebuilt
- `libllrender.a` relinked
- `Mare Viewer.app/Contents/MacOS/Mare Viewer` relinked
- manifest copy step completed
- executable verified as arm64
- `libopenal.dylib`, `libalut.dylib`, `libllwebrtc.dylib`, and
  `libndofdev.dylib` present in the app bundle

## Review Focus

Reviewers should check:

- helper names accurately describe existing behavior
- no public API was introduced
- buffer binding tracker updates stayed in the same places
- dirty-region merging and flush ordering are unchanged
- attribute layout still follows the existing shader reserved attribute order
- draw validation and `STOP_GLERROR` placement did not move
- `drawRangeFast(...)` still remains the intentionally thinner path
- no draw-pool or UI behavior was pulled into this packet
- no `llglcontainment.*` behavior was introduced

## Remaining Risk

Risk level: medium-high.

This packet improves readability, but `LLVertexBuffer` still owns fragile
OpenGL state and draw ordering. Remaining risks:

- static buffer trackers can still diverge from raw OpenGL state
- dirty mapped regions still require careful branch ordering
- attribute setup still depends on shader enum ordering and current binding
- draw calls still assume `setBuffer()` has prepared matching buffers
- runtime coverage is still limited to build/integration checks, not a full
  graphics regression suite

## Next Work

Good next candidates after phase 2:

- define the phase 3 branch and review rules before adding behavior to
  `llglcontainment.*`
- document shader program ownership before touching shader manager behavior
- document draw-pool ownership before touching `indra/newview` render paths

Avoid starting a cross-owner OpenGL containment API until one owner contract is
specific enough to justify shared behavior.
