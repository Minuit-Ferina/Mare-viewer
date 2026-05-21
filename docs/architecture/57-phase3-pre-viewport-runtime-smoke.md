# Phase 3 Pre-Viewport Runtime Smoke

This document records the runtime smoke result that unblocks the final
`LLRenderTarget` viewport containment packet.

Branch: `phase3`

Latest source checkpoint:

- `3b7fc02c5e llrender: contain render target allocation error read`

Latest summary checkpoint:

- `41b0638b29 docs: summarize render target allocation error containment`

## Runtime Result

Observed by the user after the allocation error packet:

- the Xcode-built app launches
- login page works
- a scene loads successfully
- window resize works correctly
- no immediate launch, `dyld`, scene-load, or resize failure was reported

## Decision

Viewport containment is now unblocked.

Reason:

- the previous stop point required a loaded-scene and resize smoke test before
  touching `LLRenderTarget` viewport calls
- that validation is now available

The next source task may target only:

- `glViewport(...)`

The source task must keep `LLRenderTarget` ownership of:

- render target viewport dimensions
- default framebuffer viewport restore from `gGLViewport`
- `sCurResX` and `sCurResY` updates
- `bindTarget()` / `flush()` ordering
