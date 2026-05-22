# GLTF UBO Containment Task

Branch: `phase3`
Source owners:
- `LL::GLTF::Asset`
- `LL::GLTF::Skin`

## Scope

Route direct GLTF UBO OpenGL calls through `llglcontainment.*`.

Included source files:
- `indra/newview/gltf/asset.cpp`
- `indra/newview/gltf/animation.cpp`

Included raw OpenGL families:
- `glGenBuffers`
- `glDeleteBuffers`
- `glBindBuffer`
- `glBufferData`

Included behavior:
- GLTF node matrix UBO allocation and upload
- GLTF material UBO allocation and upload
- GLTF skin matrix-palette UBO allocation, upload, and deletion

## Non-Scope

Do not change:
- GLTF parsing
- matrix packing layout
- material UBO layout
- skin joint selection
- upload timing
- buffer ownership

Do not move GLTF resource ownership into another module.

## Ownership Notes

The GLTF owners keep ownership of:
- UBO lifetime decisions
- uploaded data layout
- buffer sizes and usage flags
- matrix and material packing

`llglcontainment.*` owns only:
- direct OpenGL buffer call-through wrappers

## Risk

Risk: low.

Why:
- This is wrapper-only and uses existing containment helpers.
- Exact buffer target, size, data pointer, and usage values must stay unchanged.

## Verification

Required:
- `git diff --check`
- targeted `gltf/asset.cpp.o` build
- targeted `gltf/animation.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later with a visible GLTF object using
  material and skin UBOs.
