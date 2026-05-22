# LLDrawPoolMaterials Uniform Containment Task

Branch: `phase3`
Source owner: `LLDrawPoolMaterials`

## Scope

Route direct shader uniform OpenGL calls in
`indra/newview/lldrawpoolmaterials.cpp` through `llglcontainment.*`.

Included raw OpenGL families:
- `glUniform1f`
- `glUniform4fv`

Included behavior:
- deferred material environment intensity uniform updates
- deferred material emissive brightness uniform updates
- deferred material alpha cutoff uniform updates
- deferred material specular color uniform updates

## Non-Scope

Do not change:
- deferred material pass order
- rigged versus non-rigged shader selection
- texture binding order
- draw info iteration
- uniform location ownership
- matrix palette upload behavior

Do not move material draw-pool rendering into another owner.

## Ownership Notes

`LLDrawPoolMaterials` keeps ownership of:
- material pass selection
- cached uniform values
- texture channel selection
- draw batch iteration
- matrix palette upload

`llglcontainment.*` owns only:
- direct OpenGL uniform call-through wrappers

## Risk

Risk: low.

Why:
- This is wrapper-only and uses existing containment helpers.
- The patch must preserve exact uniform update conditions and ordering.

## Verification

Required:
- `git diff --check`
- targeted `lldrawpoolmaterials.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later with deferred materials visible.
