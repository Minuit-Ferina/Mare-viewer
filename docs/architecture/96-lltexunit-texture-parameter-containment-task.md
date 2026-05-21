# LLTexUnit Texture Parameter Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only direct texture parameter writes in `LLTexUnit`:

- `glTexParameteri(...)` in `setTextureAddressModeFast(...)`
- `glTexParameteri(...)` in `setTextureFilteringOptionFast(...)`
- `glTexParameterf(...)` in the anisotropy branch of
  `setTextureFilteringOptionFast(...)`

The owner remains `LLTexUnit`. Containment owns only the raw texture parameter
writes.

## Out Of Scope

Do not touch in this packet:

- texture binding
- texture unit activation
- texture type selection
- address mode policy
- filtering policy
- mipmap policy
- anisotropy enable/disable decisions
- `mHasMipMaps`
- `LLImageGL::sGlobalUseAnisotropic`

## Ordering Notes

The patch must preserve:

- wrap S then wrap T then optional wrap R ordering
- mag filter before min filter ordering
- existing `mHasMipMaps` branches
- existing anisotropy branch and fallback to `1.f`

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
