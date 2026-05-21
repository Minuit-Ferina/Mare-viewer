# LLGLStates Material Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only the active direct fixed-function material calls in
`indra/llrender/llglstates.h`:

- `glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ...)`
- `glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, ...)`

The owner remains `LLGLSSpecular`. Containment owns only the immediate raw
OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- material color policy
- shininess scaling
- shininess clamp range
- constructor/destructor ordering
- state class lifetime semantics
- callers of `LLGLSSpecular`

## Ordering Notes

The patch must preserve:

- material specular set before shininess set
- the existing `shininess * 128.f` conversion
- the existing `llclamp(shiny, 0, 128)` range
- material reset in the destructor only when `mShininess > 0.f`

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
