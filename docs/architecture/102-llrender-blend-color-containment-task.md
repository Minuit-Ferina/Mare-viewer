# LLRender Blend Color Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only active direct OpenGL blend/color state writes in
`LLRender`:

- `glColorMask(...)`
- `glBlendFunc(...)`
- `glBlendFuncSeparate(...)`

The owner remains `LLRender`. Containment owns only the raw state writes.

## Out Of Scope

Do not touch in this packet:

- color mask cache
- blend factor cache
- blend type policy
- `LLRender::setSceneBlendType(...)`
- flush ordering
- texture binding
- line width state
- global render initialization

## Ordering Notes

The patch must preserve:

- cache update before raw color mask write
- flush before raw blend state writes
- factor lookup through `sGLBlendFactor`
- separate color/alpha blend factor ordering

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
