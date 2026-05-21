# LLImageGL Wrapper-Pure Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers the remaining `LLImageGL` direct OpenGL calls that are
wrapper-pure: the existing `LLImageGL` code keeps ownership, ordering, policy,
and state decisions, while `llglcontainment.*` owns only the immediate raw GL
call.

Included call families:

- texture object name generation and deletion
- debug texture integer queries
- texture residency query
- readback `glGetError()` drains
- texture sub-image upload calls
- texture integer parameter calls for base/max level
- texture swizzle integer-vector parameter calls

## Out Of Scope

These call families remain contract-first and are not part of this wrapper
packet:

- compressed full texture upload
- automatic mipmap generation policy
- full `glTexImage2D(...)` texture allocation/copy
- scale-down FBO path
- scale-down PBO path
- comment-only or disabled OpenGL references

## Ownership Notes

`LLImageGL` remains responsible for:

- texture name pooling
- delayed texture deletion
- texture memory accounting
- row batching and sub-image branch selection
- debug validation and logging policy
- residency cache state
- upload, mipmap, and scale-down ordering

`LLGLContainment` is responsible only for translating the narrow wrapper calls
to OpenGL.

## Ordering Notes

The patch must preserve:

- texture name pool refill and pool miss order
- delayed free image accounting before texture deletion
- sub-image batching order and source pointer advancement
- readback error drain before and after readback
- base/max level parameter setup immediately after binding a new texture name
- swizzle parameter calls before local format conversion

## Verification Plan

- Run `git diff --check`.
- Run the targeted `llrender/fast` build.
- Regenerate `docs/architecture/generated/source_inventory.csv`.
- Compare the `LLImageGL` inventory row before and after the patch.

The local Xcode arm64 Release build is deferred for this wrapper-pure packet
unless an explicit integration checkpoint is requested. The previous Xcode
check was expensive enough that repeated full integration builds are not useful
for these call-through-only moves.
