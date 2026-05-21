# LLTexUnit Debug Query Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only one active direct OpenGL call in
`LLTexUnit::debugTextureUnit()`:

- `glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture)`

The owner remains `LLTexUnit`. Containment owns only the raw integer query.

## Out Of Scope

Do not touch in this packet:

- texture unit activation
- texture binding
- texture type state
- warning text
- `gGL.mCurrTextureUnitIndex`
- any non-debug path

## Ordering Notes

The patch must preserve:

- early return when `mIndex < 0`
- query before comparison
- expected/actual warning behavior

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
