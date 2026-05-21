# LLRender2D Line Width Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only the active direct OpenGL line-width calls in
`indra/llrender/llrender2dutils.cpp`:

- `glLineWidth(...)`
- `glGetFloatv(GL_SMOOTH_LINE_WIDTH_RANGE, ...)`

The owner remains `LLRender2D` and the local immediate-mode helper functions.
Containment owns only the raw OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- UI matrix state
- immediate-mode draw order
- color state
- line-width clamp policy
- `LLRender::sUIGLScaleFactor`
- public `LLRender2D` APIs

## Ordering Notes

The patch must preserve:

- `gGL.flush()` before direct line-width changes
- cached smooth line-width range query
- Darwin clamp behavior
- reset to width `1.f` after `gl_line_3d(...)`

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
