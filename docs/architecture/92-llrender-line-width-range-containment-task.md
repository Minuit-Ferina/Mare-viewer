# LLRender Line Width Range Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only two active direct OpenGL calls in
`LLRender::initVertexBuffer()`:

- `glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, range)`
- `glGetIntegerv(GL_SMOOTH_LINE_WIDTH_RANGE, range)`

The owner remains `LLRender`. Containment owns only the raw integer queries.

## Out Of Scope

Do not touch in this packet:

- vertex buffer allocation
- line-width clamp behavior
- `LLRender::setLineWidth(...)`
- `stop_glerror()` ordering
- texture binding
- blend or color state
- global render initialization outside these two queries

## Ordering Notes

The patch must preserve:

- the existing `stop_glerror()` before the first query
- `stop_glerror()` after each query
- assignment to `mMaxLineWidthAliased` before the smooth range query
- assignment to `mMaxLineWidthSmooth` after the smooth range query

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
