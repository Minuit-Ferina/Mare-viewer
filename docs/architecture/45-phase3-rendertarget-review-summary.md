# Phase 3 Render Target Review Summary

This document reviews all phase 3 `LLRenderTarget` containment packets
completed so far before selecting another OpenGL family.

Branch: `phase3`

Base branch: `phase2`

## Packets Reviewed

Completed phase 3 `LLRenderTarget` packets:

- FBO bind/status containment
- FBO texture attachment containment
- draw/read buffer routing containment

## Review Result

No blocking issue found in the combined source diff.

The packets remain within the intended phase 3 boundary:

- source changes are limited to `llglcontainment.*` and `llrendertarget.cpp`
- `LLRenderTarget` remains the owner of render target state and ordering
- `LLGLContainment` owns only raw OpenGL calls selected by task notes
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
- depth-only `GL_NONE` routing
- default framebuffer `GL_BACK` restore policy

## Verification Evidence

Recorded checks:

- targeted `llrender/fast`: passed for all source packets
- generated inventory regenerated after each source packet
- local incremental Xcode arm64 Release build: passed for each source packet
- executable verified as arm64
- runtime dylibs verified in the app bundle
- Xcode-built app launched to the login screen
- a loaded scene was opened successfully after the buffer routing packet

Remaining runtime gap:

- no formal FPS recapture
- no broad graphics regression pass
- no focused checks for dynamic textures, reflection probes, or preview widgets

## Next Candidate

The next suitable `LLRenderTarget` candidate is FBO name lifetime:

- `glGenFramebuffers(...)`
- `glDeleteFramebuffers(...)`

Reason:

- phase 2 contract already exists
- direct calls are concentrated in `generate_framebuffer_name(...)` and
  `delete_framebuffer_name(...)`
- `LLRenderTarget` can keep ownership of `mFBO`, release ordering, tracker
  cleanup, and field reset
- the task stays within the same owner and does not require touching
  `pipeline.cpp`

Before source edits, add a task note for this FBO lifetime family.
