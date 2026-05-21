# LLRender Clear Errors Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only one active direct OpenGL call in
`LLRender::clearErrors()`:

- `glGetError()`

The owner remains `LLRender`. Containment owns only the raw error read.

## Out Of Scope

Do not touch in this packet:

- loop shape
- error handling policy
- any other `glGetError()` callsite
- render initialization
- blend/color/line state

## Ordering Notes

The patch must preserve the current loop:

- repeatedly read the error flag
- stop when the read returns no error
- do not log or transform the error value

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
