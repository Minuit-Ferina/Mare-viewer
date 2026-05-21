# LLTexUnit Binding Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only texture unit activation and texture binding calls in
`LLTexUnit`:

- `glActiveTexture(...)`
- `glBindTexture(...)`

The owner remains `LLTexUnit`. Containment owns only the raw OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- texture cache fields
- texture unit cache fields
- `gGL.mCurrTextureUnitIndex`
- `gGL.mDirty`
- `gGL.flush()` ordering
- `activate()` / `enable()` control flow
- missing texture fallback behavior
- bind stats
- texture parameter policy
- render target binding policy

## Ordering Notes

The patch must preserve:

- every existing `gGL.flush()` position
- every existing `activate()` and `enable(...)` call
- active texture call before cache assignment in `bindFast(...)`
- texture name assignment before bind in existing code paths
- white texture fallback for `TT_TEXTURE`
- zero binding for non-`TT_TEXTURE` unbind paths

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
