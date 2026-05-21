# LLImageGL ScaleDown PBO Storage Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only the remaining PBO-style storage calls in
`LLImageGL::scaleDown(...)`:

- PBO-path `glTexImage2D(..., nullptr)`
- PBO-path `glGenerateMipmap(...)`

The owner remains `LLImageGL::scaleDown(...)`. Containment owns only the raw
OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- scratch PBO creation
- scratch PBO resize policy
- pack/unpack binding order
- texture memory accounting
- discard-level mutation
- bind/unbind order
- FBO-path behavior

## Ordering Notes

The patch must preserve:

- pack PBO readback before old texture memory accounting is freed
- pack PBO unbind before unpack PBO bind
- unpack PBO bind before level 0 reallocation
- unpack PBO unbind after level 0 reallocation
- `alloc_tex_image(...)` after unpack reallocation
- conditional mipmap regeneration before texture unbind

## Verification Plan

- Run `git diff --check`.
- Run the targeted `llrender/fast` build.
- Regenerate source inventory.
- Confirm active direct `LLImageGL` OpenGL calls are gone, leaving only
  inactive/comment-only matches.
