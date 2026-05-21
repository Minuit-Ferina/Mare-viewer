# LLRenderTarget Phase 2 Review Summary

This document is the review entry point for the phase 2 `LLRenderTarget`
intent-naming packet.

Related source files:

- `indra/llrender/llrendertarget.cpp`
- `indra/llrender/llrendertarget.h`

Related summary:

- `docs/architecture/24-llrendertarget-local-intent-map.md`

## Purpose

The packet makes existing OpenGL intent inside `LLRenderTarget` easier to
review without changing runtime behavior.

It does this by:

- documenting each OpenGL call family before or alongside the source cleanup
- replacing direct callsites with local `.cpp` helpers whose names describe the
  existing local intent
- keeping ownership inside `LLRenderTarget`
- avoiding public API changes
- avoiding `llglcontainment.*` until there is a phase 3 cross-owner contract

## Source Change Scope

Touched source:

- `indra/llrender/llrendertarget.cpp`

Not touched:

- `indra/llrender/llrendertarget.h`
- `indra/llrender/llglcontainment.*`
- `indra/newview/pipeline.cpp`
- draw pools
- UI rendering paths
- shader managers
- texture upload owners outside `LLRenderTarget`

## Commit Packet

The source-side `LLRenderTarget` packet is:

- `03adb52f1b docs: document render target viewport contract`
- `9cc00c5889 llrender: name render target viewport intents`
- `cd69272c10 docs: document render target fbo contract`
- `801eb31041 llrender: name render target fbo intents`
- `4c095a9477 llrender: name render target buffer routing intents`
- `7fdf1ddbac llrender: name render target attachment intents`
- `2d3da780ef llrender: name render target fbo lifetime intents`
- `dd084831c3 llrender: name render target mipmap intent`
- `7eff81cbb5 llrender: name render target clear intents`
- `ca85c5f7cc llrender: name render target framebuffer status intent`
- `f954327a7d llrender: name render target allocation error intent`
- `82e183aad6 docs: summarize render target local intents`
- `480a39fc4b docs: record render target xcode integration check`

## Intent Families Covered

The packet covers these existing `LLRenderTarget` OpenGL families:

- framebuffer status checks
- viewport set and restore
- FBO draw-time binding
- temporary FBO attachment mutation binding
- FBO name lifetime
- texture attach and detach
- draw/read buffer routing
- mipmap generation
- clear and scissor fallback
- texture allocation error checks

Each family has its own contract document under `docs/architecture/`.

## Behavior Expectations

Expected behavior change:

- none

The patch should preserve:

- `bindTarget()` / `flush()` pairing
- render target stack behavior through `sBoundTarget`
- FBO tracking through `sCurFBO`
- default viewport restore through `gGLViewport`
- draw/read buffer routing
- texture attachment and deletion order
- existing `gDebugGL` framebuffer status behavior
- existing `glGetError()` consumption points after texture allocation attempts

## Why Helpers Stay Local

The helpers intentionally stay in `llrendertarget.cpp`.

Reason:

- most helpers depend on `LLRenderTarget` fields or static state
- the ordering is meaningful only inside `LLRenderTarget`
- moving these helpers to `llglcontainment.*` now would create a generic
  OpenGL wrapper before a reusable containment contract exists

`llglcontainment.*` remains reserved for a later phase where a cross-owner
contract is explicit.

## Verification Performed

Targeted builds:

- `llrender/fast` after each source-side packet

Observed targeted build result:

- `llrendertarget.cpp.o` rebuilt
- `libllrender.a` relinked
- no clean viewer build was run for the small source packets

Integration build:

- local Xcode arm64 Release build
- incremental, no `clean`
- command and result recorded in
  `docs/architecture/local-darwin-arm64-build.md`

Observed integration result:

- `** BUILD SUCCEEDED **`
- `llrendertarget.cpp.o` rebuilt
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
- no helper accidentally changes `sCurFBO` update points
- attachment mutation still restores the tracked FBO binding
- default framebuffer restore still restores viewport and buffer routing
- texture allocation error checks still happen at the same points
- no `pipeline.cpp` or draw-pool behavior was pulled into this packet

## Remaining Risk

Risk level: medium.

This packet improves readability, but the underlying render target behavior is
still global OpenGL state. Remaining risks:

- nested render target ordering is still fragile
- `sCurFBO` and raw OpenGL binding can still diverge if future edits are careless
- `gGLViewport` freshness is still owned outside `LLRenderTarget`
- texture allocation error handling still consumes global GL error state
- runtime coverage is still limited to build/integration checks, not a full
  graphics regression suite

## Next Work

Good next phase 2 candidates:

- document `LLImageGL` texture upload and lifetime call families
- document `LLVertexBuffer` buffer binding and update call families
- document `LLGLSLShader` shader program call families

Avoid starting phase 3 containment extraction until one of those owners reveals
a stable cross-owner contract.
