# LLShaderMgr Containment Task

Branch: `phase3`
Source owner: `LLShaderMgr`

## Scope

Route active direct OpenGL calls in `indra/llrender/llshadermgr.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- shader/program log queries
- shader/program status queries
- shader creation and deletion
- shader source upload and compile
- program link and validate
- program binary cache load/save
- GL error reads

## Non-Scope

Do not change:
- shader feature attach order
- shader source preprocessing
- shader retry and fallback behavior
- error handling
- log formatting
- shader binary cache file format
- shader cache eviction behavior

## Ownership Notes

`LLShaderMgr` keeps ownership of:
- source file loading
- generated shader source lines
- compile/link retry policy
- shader cache lookup and persistence
- shader feature selection
- logging decisions

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers
- count and pointer type conversion at the OpenGL boundary

## Risk

Risk: medium.

Why:
- The patch is wrapper-only, but this file owns shader compile/link and cached
  program binary behavior.
- Any changed order around compile, link, or binary load would affect shader
  startup.

Guardrail:
- Preserve every conditional, error branch, retry branch, and log branch.

## Verification

Required:
- `git diff --check`
- targeted `llrender/fast`
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- recommended at a later checkpoint because shader startup paths are touched,
  even though this packet is wrapper-only.

