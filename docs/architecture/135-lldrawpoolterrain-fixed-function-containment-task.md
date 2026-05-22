# LLDrawPoolTerrain Fixed Function Containment Task

Branch: `phase3`
Source owner: `LLDrawPoolTerrain`

## Scope

Route fixed-function terrain OpenGL calls in
`indra/newview/lldrawpoolterrain.cpp` through `llglcontainment.*`.

Included raw OpenGL families:
- `glPolygonOffset`
- `glEnable`
- `glDisable`
- `glTexGeni`
- `glTexGenfv`

Included behavior:
- parcel ownership polygon offset
- legacy texture coordinate generation for terrain detail passes

## Non-Scope

Do not change:
- terrain pass order
- texture unit activation order
- texture bindings
- texture matrix setup
- blend state
- shader selection
- draw loop behavior

## Ownership Notes

`LLDrawPoolTerrain` keeps ownership of:
- terrain rendering pass sequencing
- texture unit selection
- generated texture coordinate planes
- detail texture and alpha ramp binding
- parcel ownership highlight rendering

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers
- fixed-function state API names

## Risk

Risk: medium.

Why:
- This is wrapper-only, but it touches legacy terrain fixed-function state.
- The patch must preserve exact ordering around texture unit activation.

## Verification

Required:
- `git diff --check`
- targeted/incremental newview build check
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- recommended at a later checkpoint because this path affects terrain passes.

