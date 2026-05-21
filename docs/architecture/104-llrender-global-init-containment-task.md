# LLRender Global Init Containment Task

Branch: `phase3`

Base branch: `phase2`

## Scope

This task covers only fixed global initialization calls in `LLRender::init(...)`:

- `glPixelStorei(GL_PACK_ALIGNMENT, 1)`
- `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)`
- `glCullFace(GL_BACK)`
- `glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS)`
- `glGenVertexArrays(1, &ret)`
- `glBindVertexArray(ret)`

The owner remains `LLRender`. Containment owns only the raw OpenGL calls.

## Out Of Scope

Do not touch in this packet:

- Windows debug callback setup
- `glDebugMessageCallback(...)`
- `glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS)`
- scene blend setup
- ambient light setup
- vertex buffer allocation
- texture binding or activation paths

## Ordering Notes

The patch must preserve:

- pixel-store setup before scene blend setup
- cull face setup after scene blend and ambient light setup
- cube-map seamless enable before dummy VAO setup
- existing Windows null check for `glGenVertexArrays`
- dummy VAO generation before dummy VAO bind

## Verification Plan

- Run `git diff --check`.
- Run targeted `llrender/fast`.
- Regenerate source inventory.

No Xcode or runtime smoke is required for this pure wrapper packet.
