# LLImageGL ScaleDown FBO Storage Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only the remaining FBO-style storage calls in
`LLImageGL::scaleDown(...)`:

- FBO-path `glTexImage2D(..., nullptr)`
- FBO-path `glGenerateMipmap(...)`

The owner remains `LLImageGL::scaleDown(...)`. Containment owns only the raw
OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- PBO-path texture reallocation
- PBO-path mipmap regeneration
- framebuffer-to-texture copy
- texture memory accounting
- `mTexOptionsDirty`
- discard-level mutation
- bind/unbind order

## Ordering Notes

The patch must preserve:

- `free_tex_image(mTexName)` before level 0 reallocation
- framebuffer copy immediately after level 0 reallocation
- `alloc_tex_image(...)` after framebuffer copy
- `mTexOptionsDirty = true` after accounting allocation
- conditional mipmap regeneration after rebinding this texture
- texture unbind after mipmap generation

## Verification Plan

- Run `git diff --check`.
- Run the targeted `llrender/fast` build.
- Regenerate source inventory.
- Confirm PBO-path reallocation and mipmap calls remain direct for the next
  packet.
