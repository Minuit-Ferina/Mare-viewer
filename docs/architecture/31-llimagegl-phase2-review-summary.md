# LLImageGL Phase 2 Review Summary

This document is the review entry point for the phase 2 `LLImageGL`
intent-naming packet.

Related source files:

- `indra/llrender/llimagegl.cpp`

Related documents:

- `docs/architecture/26-llimagegl-texture-lifecycle-map.md`
- `docs/architecture/27-llimagegl-texture-name-lifetime-contract.md`
- `docs/architecture/28-llimagegl-pixel-store-contract.md`
- `docs/architecture/29-llimagegl-readback-copy-contract.md`
- `docs/architecture/30-llimagegl-pbo-sync-contract.md`

## Purpose

The packet makes existing OpenGL intent inside `LLImageGL` easier to review
without changing runtime behavior.

It does this by:

- documenting each touched OpenGL call family before source cleanup
- replacing selected direct callsites with local `.cpp` helpers whose names
  describe existing local intent
- keeping ownership inside `LLImageGL`
- avoiding public API changes
- avoiding `llglcontainment.*` until there is a phase 3 cross-owner contract

## Source Change Scope

Touched source:

- `indra/llrender/llimagegl.cpp`

Not touched:

- `indra/llrender/llimagegl.h`
- `indra/llrender/llglcontainment.*`
- `indra/newview/`
- texture fetch/cache owners outside `LLImageGL`
- UI rendering paths
- shader managers

## Commit Packet

The `LLImageGL` packet is:

- `562fe0340a docs: map llimagegl texture lifecycle`
- `03f4955db6 docs: document llimagegl texture name lifetime`
- `ef40d4d71f docs: document llimagegl pixel store contract`
- `bbcbb73b41 llrender: name llimagegl pixel store intents`
- `14590767ab docs: document llimagegl readback copy contract`
- `c8c5d66684 llrender: name llimagegl readback copy intents`
- `8cfa213b1e docs: document llimagegl pbo sync contract`
- `9c8d92a86e llrender: name llimagegl scratch pbo intents`
- `7ced5dfbe0 llrender: name llimagegl sync intents`
- `a0ac5b981f docs: record llimagegl xcode integration check`

## Intent Families Covered

The packet covers these existing `LLImageGL` OpenGL families:

- texture name lifecycle, documented only
- pixel store state for `GL_UNPACK_SWAP_BYTES` and `GL_UNPACK_ROW_LENGTH`
- texture readback queries and read calls
- current-framebuffer-to-texture copy calls
- scratch PBO creation, deletion, pack/unpack binding, and resize
- texture upload sync object creation, waits, flushes, and deletion

The packet intentionally does not cover:

- normal full texture upload through `glTexImage2D`
- partial CPU texture update through `glTexSubImage2D`
- mipmap generation policy
- texture parameter and swizzle policy
- debug-only texture size checks

## Behavior Expectations

Expected behavior change:

- none

The patch should preserve:

- `mFormatSwapBytes` enable/reset pairing
- `GL_UNPACK_ROW_LENGTH` reset after partial updates
- `readBackRaw(...)` validation, allocation, and GL error behavior
- framebuffer copy binding and `mGLTextureCreated` update behavior
- `scaleDown(...)` method selection through `gGLManager.mDownScaleMethod`
- scratch PBO size tracking
- texture memory accounting order around scale-down reallocations
- NVIDIA versus non-NVIDIA sync branching
- main-thread callback order for sync wait and texture-name handoff
- `ref()` / `unref()` lifetime around the texture-name callback

## Why Helpers Stay Local

The helpers intentionally stay in `llimagegl.cpp`.

Reason:

- pixel store behavior depends on `LLImageGL` upload state
- readback owns `LLImageRaw` allocation and failure behavior
- scratch PBO state is `LLImageGL` class state
- sync behavior depends on `LLImageGL` texture-name handoff and queue ordering
- moving these helpers to `llglcontainment.*` now would create a generic
  OpenGL wrapper before a reusable containment contract exists

`llglcontainment.*` remains reserved for a later phase where a cross-owner
contract is explicit.

## Verification Performed

Targeted builds:

- `llrender/fast` after each source-side packet

Observed targeted build result:

- `llimagegl.cpp.o` rebuilt
- `libllrender.a` relinked
- no clean viewer build was run for the small source packets

Integration build:

- local Xcode arm64 Release build
- incremental, no `clean`
- command and result recorded in
  `docs/architecture/local-darwin-arm64-build.md`

Observed integration result:

- `** BUILD SUCCEEDED **`
- `llimagegl.cpp.o` rebuilt
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
- pixel store resets remain in the same branches as before
- readback allocation and error-drain behavior is unchanged
- PBO pack binding is reset before unpack binding
- texture memory accounting remains around the same scale-down operations
- NVIDIA and non-NVIDIA sync paths still differ in the same way
- the posted sync wait callback still precedes the texture-name callback
- no `llglcontainment.*` behavior was introduced

## Remaining Risk

Risk level: medium-high.

This packet improves readability, but `LLImageGL` still owns several fragile
OpenGL state families. Remaining risks:

- global pixel store state can still leak if future branches add early returns
- scratch PBO pack/unpack state is still global OpenGL state
- `scaleDown(...)` still mixes viewport, draw, texture allocation, PBO, and
  texture memory accounting
- texture upload sync still depends on vendor-specific behavior and queue order
- runtime coverage is still limited to build/integration checks, not a full
  graphics regression suite

## Next Work

Good next phase 2 candidates:

- document `LLImageGL` full upload, mipmap, and texture-parameter call families
- document `LLVertexBuffer` buffer binding and update call families
- document `LLGLSLShader` shader program call families

Avoid starting phase 3 containment extraction until one of those owners reveals
a stable cross-owner contract.
