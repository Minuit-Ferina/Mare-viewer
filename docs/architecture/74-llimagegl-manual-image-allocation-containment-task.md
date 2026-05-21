# LLImageGL Manual Image Allocation Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only the two active `glTexImage2D(...)` calls inside
`LLImageGL::setManualImage(...)`:

- combined allocation and copy when staggered upload is not used
- allocation with `nullptr` before `sub_image_lines(...)` when staggered upload
  is used

The owner remains `LLImageGL::setManualImage(...)`. Containment owns only the
raw `glTexImage2D(...)` call.

## Out Of Scope

Do not touch in this packet:

- `LLImageGL::scaleDown(...)`
- texture memory accounting
- `free_cur_tex_image()` or `alloc_tex_image(...)`
- format conversion and compression remapping
- `sub_image_lines(...)` batching behavior
- alpha analysis or pick-mask updates in callers
- public `LLImageGL` APIs

## Ownership Notes

`LLImageGL::setManualImage(...)` remains responsible for:

- deciding whether compression is allowed
- converting deprecated formats on core profile
- selecting staggered upload versus single allocation/copy
- freeing existing texture accounting before allocation
- allocating new texture accounting after allocation/copy
- preserving the profiling zones around allocation and copy

`LLGLContainment` may add only a narrow uncompressed texture image helper.

## Ordering Notes

The patch must preserve:

- `stop_glerror()` before entering the allocation block
- `free_cur_tex_image()` before any level allocation
- `should_stagger_image_set(...)` branch selection before the upload call
- single-call allocation/copy when staggered upload is disabled
- allocation with `nullptr` before `sub_image_lines(...)` when staggered upload
  is enabled
- `alloc_tex_image(...)` after allocation/copy
- `stop_glerror()` after the allocation block

## Verification Plan

- Run `git diff --check`.
- Run the targeted `llrender/fast` build.
- Regenerate `docs/architecture/generated/source_inventory.csv`.
- Confirm the only active direct `LLImageGL` calls left are in
  `scaleDown(...)`, plus inactive/comment-only matches.

The local Xcode arm64 Release build and runtime smoke test remain deferred
unless an integration checkpoint is explicitly requested. This packet changes
only call routing, not allocation order or memory accounting.
