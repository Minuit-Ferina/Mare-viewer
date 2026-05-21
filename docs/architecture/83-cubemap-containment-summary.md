# Cubemap Containment Summary

Branch: `phase3`

Base branch: `phase2`

Source commits:

- `7a7e9bd5b9` `LLCubeMap`
- `f87e6bebbf` `LLCubeMapArray`

## Completed Scope

Active direct OpenGL calls in these files now route through
`llglcontainment.*`:

- `indra/llrender/llcubemap.cpp`
- `indra/llrender/llcubemaparray.cpp`

Moved `LLCubeMap` calls:

- seamless cubemap enable
- cubemap mipmap generation

Moved `LLCubeMapArray` calls:

- cubemap array readback
- cubemap array sub-image upload
- cubemap array 3D storage allocation

The owner files still own texture name lifetime, binding order, mip allocation
loops, source image scaling, texture memory accounting, and public APIs.

## Inventory Result

Generated source inventory after the packets:

- `indra/llrender/llcubemap.cpp`: active direct `gl*` calls dropped to 0
- `indra/llrender/llcubemaparray.cpp`: active direct `gl*` calls dropped to 0
- `indra/llrender/llcubemaparray.cpp` still has one generated match from the
  disabled AMD-driver `glGenerateMipmap(...)` comment

## Verification

Completed for each source packet:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

No Xcode or runtime smoke was run because both packets are pure wrapper
routing.

## Next Candidate

Recommended next small owner:

- `indra/llrender/llrender2dutils.cpp`

Reason:

- only three direct active calls remain there
- the calls appear to be line-width query/set wrappers
- it is smaller and safer than shader management, `llrender.cpp`, or
  `pipeline.cpp`
