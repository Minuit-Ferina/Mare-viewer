# LLCubeMap Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only the active direct OpenGL calls in
`indra/llrender/llcubemap.cpp`:

- `glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS)`
- `glGenerateMipmap(GL_TEXTURE_CUBE_MAP)`

The owner remains `LLCubeMap`. Containment owns only the raw OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- cubemap texture name ownership
- `LLImageGL` face creation
- cubemap face upload
- texture unit binding order
- environment map image ownership
- public `LLCubeMap` APIs

## Ordering Notes

The patch must preserve:

- seamless cubemap enable after environment map bind/filter setup
- mipmap generation after the cubemap is bound
- texture unit disable and `LLCubeMap::disable()` order

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
