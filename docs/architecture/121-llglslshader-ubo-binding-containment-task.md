# LLGLSLShader UBO Binding Containment Task

Branch: `phase3`
Source owner: `LLGLSLShader`

## Scope

Route only the uniform block binding call in
`LLGLSLShader::mapUniforms()` through `llglcontainment.*`.

Included raw OpenGL family:
- `glUniformBlockBinding`

Included callsite:
- the UBO setup loop for `ReflectionProbes`, `GLTFJoints`, `GLTFNodes`, and
  `GLTFMaterials`

## Non-Scope

Do not route in this packet:
- `glUseProgram`
- shader lifecycle calls
- shader attach/detach/delete calls
- attribute binding before link
- inactive `DEBUG_SHADER_INCLUDES` code

## Ownership Notes

`LLGLSLShader` keeps ownership of:
- UBO block names
- block ordering
- the `NUM_UNIFORM_BLOCKS` assertion
- the `GL_INVALID_INDEX` guard
- the Apple-compatible UBO binding policy

`llglcontainment.*` owns only:
- direct `glUniformBlockBinding(...)` call-through.

## Ordering Contract

The source patch must preserve:

1. `bind()` before uniform and UBO mapping.
2. `getUniformBlockIndex(...)` before binding each block.
3. `GL_INVALID_INDEX` guard before binding.
4. `unbind()` after the UBO setup loop.

## Risk

Risk: medium.

Why:
- It is a single call-through wrapper, but it writes shader program state.
- The surrounding ordering is important for GLTF and reflection probe uniform
  blocks.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred for this isolated wrapper move.

