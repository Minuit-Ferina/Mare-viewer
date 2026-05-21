# Phase 3 FBO Containment Review

This document records the review result for the first phase 3 FBO
containment packet before another OpenGL family is moved into
`llglcontainment.*`.

Reviewed packet:

- `33c5618af9 docs: start phase3 containment plan`
- `8ca875afa8 docs: define first phase3 fbo containment task`
- `bffb117844 llrender: contain render target fbo calls`
- `582c948ceb docs: record phase3 fbo containment check`
- `b3abe58f67 docs: summarize phase3 fbo containment packet`

## Review Result

No blocking issue found in the source diff.

The packet matches the phase 3 task boundary:

- only `llglcontainment.*` and `llrendertarget.cpp` are touched for source
- only raw `glBindFramebuffer(GL_FRAMEBUFFER, ...)` and
  `glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)` moved into
  `LLGLContainment`
- `LLRenderTarget` local helpers still express render target intent
- `sCurFBO` assignments remain in `LLRenderTarget`
- attachment mutation binding still does not update `sCurFBO`
- `gDebugGL`, warning text, and `ll_fail(...)` remain in `LLRenderTarget`
- no public `LLRenderTarget` API changed
- no `pipeline.cpp`, draw pool, UI, shader, or texture upload code changed

## Verification Rechecked

Previously recorded checks remain the relevant evidence:

- targeted `llrender/fast`: passed
- local incremental Xcode arm64 Release build: passed
- executable verified as arm64
- runtime dylibs verified in the app bundle
- generated source inventory regenerated

Additional local check before this review note:

- source diff inspected from `phase2..phase3`
- FBO containment callsites inspected with `rg`
- `git diff --check`: passed

## Residual Risk

Risk level: medium.

The remaining risk is runtime coverage, not source shape. The packet has not
yet been manually smoke-tested in the viewer after launch. Areas to watch in a
runtime check:

- login screen render target setup
- window resize
- default framebuffer restore
- loaded scene rendering
- dynamic textures, reflection probes, and preview widgets

## Decision

It is reasonable to prepare the next phase 3 task, as long as the next source
packet remains similarly narrow and has its own task note before code edits.
