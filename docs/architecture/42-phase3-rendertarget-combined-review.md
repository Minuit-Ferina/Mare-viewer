# Phase 3 Render Target Combined Review

This document reviews the phase 3 `LLRenderTarget` containment work completed
so far before selecting another OpenGL family.

Reviewed packets:

- FBO bind/status containment
- FBO texture attachment containment

## Review Result

No blocking issue found in the combined source diff.

The packets remain within the intended phase 3 boundary:

- source changes are limited to `llglcontainment.*` and `llrendertarget.cpp`
- `LLRenderTarget` remains the owner of render target intent and state
- `LLGLContainment` owns only raw OpenGL calls already selected by task notes
- no public `LLRenderTarget` API changed
- no `pipeline.cpp`, draw pool, UI, shader, or texture upload code changed

## Ownership Still Local

The following still live in `LLRenderTarget`:

- `sCurFBO` updates
- `sBoundTarget` stack behavior
- render target binding versus attachment mutation intent
- default framebuffer restore ordering
- `gDebugGL`, warning text, and `ll_fail(...)`
- depth versus color attachment selection
- attachment index math
- `LLTexUnit::getInternalType(...)`
- texture detach convention using name `0`

## Verification Evidence

Recorded checks:

- targeted `llrender/fast`: passed for both source packets
- generated inventory regenerated after both source packets
- local incremental Xcode arm64 Release build: passed for both source packets
- executable verified as arm64
- runtime dylibs verified in the app bundle
- Xcode-built app launched to the login screen

Remaining runtime gap:

- no loaded-scene smoke test has been recorded yet

## Next Candidate

The next suitable `LLRenderTarget` candidate is buffer routing:

- `glDrawBuffer(...)`
- `glReadBuffer(...)`
- `glDrawBuffers(...)`

Reason:

- phase 2 contract already exists
- direct calls are concentrated in `set_render_target_buffer_routing(...)` and
  `restore_default_framebuffer_buffer_routing()`
- the local owner still decides depth-only routing, color attachment count, and
  default back-buffer restore
- the task stays within the same owner and does not require touching
  `pipeline.cpp`

Before source edits, add a task note for this routing family.
