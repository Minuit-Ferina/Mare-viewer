# LLCubeMapArray Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only the active direct OpenGL calls in
`indra/llrender/llcubemaparray.cpp`:

- `glGetTexImage(...)`
- `glTexSubImage3D(...)`
- `glTexImage3D(...)`

The owner remains `LLCubeMapArray`. Containment owns only the raw OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- cubemap array allocation policy
- mip level loop logic
- source image scaling
- texture memory accounting
- texture unit binding order
- AMD mipmap-generation comment
- public `LLCubeMapArray` APIs

## Ordering Notes

The patch must preserve:

- readback before source image scaling
- scaled image upload into the same array layer index
- `free_cur_tex_image()` before the allocation loop
- `alloc_tex_image(...)` after the allocation loop
- the existing manual mip allocation loop

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
