# LLRender Global Init Containment Summary

Branch: `phase3`

Base branch: `phase2`

Source commit: `f0b07e9a91`

## Completed Scope

Fixed global initialization calls in `LLRender::init(...)` now route through
`llglcontainment.*`.

Moved calls:

- pixel-store pack alignment
- pixel-store unpack alignment
- cull face setup
- cube-map seamless enable
- dummy vertex array generation
- dummy vertex array bind

`LLRender` still owns startup ordering, scene blend setup, ambient light setup,
the Windows `glGenVertexArrays` availability check, and vertex buffer
initialization.

## Intentionally Left Direct

The Windows debug callback setup remains direct:

- `glDebugMessageCallback(...)`
- `glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS)`

Reason: it is platform-specific debug callback wiring and should be reviewed in
a separate Windows-focused packet if needed.

## Inventory Result

Generated source inventory after the packet:

- `indra/llrender/llrender.cpp`: active direct `gl*` calls dropped from 23 to 17
- `indra/llrender/llglcontainment.cpp`: likely `gl*` call expressions rose
  from 64 to 67

## Verification

Completed:

- `git diff --check`
- targeted `llrender/fast` build
- regenerated `docs/architecture/generated/source_inventory.csv`
- regenerated `docs/architecture/generated/source_inventory_top.md`

No Xcode or runtime smoke was run because this is pure wrapper routing.

## Next Candidate

Remaining `llrender.cpp` source routing candidates are now mostly
`LLTexUnit` binding/activation calls. Those are central but still mechanically
wrappable if handled in one focused packet that preserves every `gGL.flush()`,
`activate()`, cache assignment, and fallback binding path.
