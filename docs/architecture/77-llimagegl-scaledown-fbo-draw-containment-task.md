# LLImageGL ScaleDown FBO Draw Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only the FBO-style draw setup calls in
`LLImageGL::scaleDown(...)`:

- `glViewport(...)`
- `glDrawArrays(...)`

The owner remains `LLImageGL::scaleDown(...)`. Containment owns only the raw
OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- FBO texture reallocation
- FBO mipmap regeneration
- framebuffer-to-texture copy
- PBO-path reallocation
- PBO-path mipmap regeneration
- texture memory accounting
- discard-level mutation
- texture bind failure handling

## Ordering Notes

The patch must preserve:

- viewport setup before FBO draw binding/draw work
- texture bind success as the gate for draw submission
- full-screen triangle draw before texture reallocation and framebuffer copy
- existing FBO and PBO path selection

## Verification Plan

- Run `git diff --check`.
- Run the targeted `llrender/fast` build.
- Regenerate source inventory.
- Confirm the FBO reallocation/mipmap and PBO calls remain direct for later
  packets.
