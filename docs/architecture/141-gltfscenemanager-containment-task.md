# GLTFSceneManager Containment Task

Branch: `phase3`
Source owner: `GLTFSceneManager`

## Scope

Route direct OpenGL calls in `indra/newview/gltfscenemanager.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- `glBindBufferBase`
- `glActiveTexture`
- `glBindTexture`
- `glTexParameteri`
- `glPolygonMode`

Included behavior:
- GLTF node/material/joint UBO binding
- GLTF material texture unit activation
- GLTF texture binding and sampler parameter setup
- GLTF debug raycast wireframe overlay

## Non-Scope

Do not change:
- GLTF asset ownership
- shader variant selection
- material binding order
- texture channel selection
- sampler fallback behavior
- vertex buffer draw order
- debug traversal behavior

Do not move GLTF rendering into another owner.

## Ownership Notes

`GLTFSceneManager` keeps ownership of:
- GLTF render batch iteration
- UBO slot selection
- material and texture selection
- sampler parameter values
- debug raycast overlay ordering

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers
- API names for buffer, texture, and polygon-mode operations

## Risk

Risk: low to medium.

Why:
- This is wrapper-only, but it touches live GLTF material binding.
- Exact texture unit, sampler, and UBO ordering must stay unchanged.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `gltfscenemanager.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; useful later with a visible GLTF object and raycast
  debug overlay.
