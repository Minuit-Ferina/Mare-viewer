# LLTexUnit Binding Containment Summary

Branch: `phase3`

Base branch: `phase2`

Source commit: `0e01fbf8b7`

## Completed Scope

`LLTexUnit` texture unit activation and texture binding calls now route through
`llglcontainment.*`.

Moved calls:

- active texture selection
- texture binding
- white texture fallback binding
- zero texture unbind paths
- cube-map binding
- manual render-target texture binding path

`LLTexUnit` still owns texture unit state, texture type state, cache updates,
fallback behavior, bind stats, `gGL.flush()` ordering, and `activate()` /
`enable(...)` control flow.

## Inventory Result

Generated source inventory after the packet:

- `indra/llrender/llrender.cpp`: active direct `gl*` calls dropped from 17 to 3
- `indra/llrender/llglcontainment.cpp`: likely `gl*` call expressions rose
  from 67 to 69

The remaining active direct calls in `llrender.cpp` are the Windows debug
callback setup:

- `glDebugMessageCallback(...)`
- `glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS)`

The generated inventory also counts the commented
`glDebugMessageControl(...)` line as a raw reference.

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

No Xcode or runtime smoke was run because this is pure wrapper routing.

## Next Candidate

Recommended next step:

- stop `llrender.cpp` source routing here
- handle the Windows debug callback block only in a separate platform-specific
  task, if needed

Reason: the remaining direct calls are not normal render-path calls; they wire a
debug callback during Windows initialization.
