# LLRender Set Line Width Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only active direct OpenGL calls in
`LLRender::setLineWidth(...)`:

- `glIsEnabled(GL_LINE_SMOOTH)`
- `glLineWidth(line_width)`

The owner remains `LLRender`. Containment owns only the raw capability query
and line-width write.

## Out Of Scope

Do not touch in this packet:

- core-profile clamp to `1.f`
- line-smooth clamp policy
- cached max line widths
- `mLineWidth`
- `mDirty`
- flush ordering
- render mode checks

## Ordering Notes

The patch must preserve:

- core-profile early clamp before querying line smooth state
- line-smooth query only when `line_width > 1.f`
- flush before changing width for line render modes
- cached width assignment before the raw line-width write

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
