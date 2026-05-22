# LLGLContainment Contract

Branch: `phase3`

## Status

`LLGLContainment` is now the only accepted runtime OpenGL call boundary for the
contained renderer work completed in phase 3.

Current snapshot:

- `indra/llrender/llglcontainment.h`: 162 public declarations.
- `indra/llrender/llglcontainment.cpp`: 160 runtime `gl*` calls.
- `tools/architecture/check_gl_containment.py`: zero runtime `gl*` calls
  outside `llglcontainment.cpp`.

This is still containment, not a renderer rewrite. The wrappers preserve the
existing call order, caller ownership, and platform behavior.

## Allowed Ownership

`LLGLContainment` may own:

- direct calls to OpenGL entry points
- simple type normalization at the OpenGL ABI boundary
- local platform guards for entry points unavailable on a platform, such as
  Darwin OpenGL compute exclusions
- simple return conversion for GL query APIs
- wrapper names that expose caller intent more clearly than the raw GL symbol

It may not own:

- render pass orchestration
- framebuffer, texture, shader, buffer, or query lifetimes
- implicit bind/restore behavior not present in the caller before containment
- hidden GL error handling policy
- hidden logging or performance instrumentation
- UI/render coupling decisions
- Vulkan, Metal, SDL, or window-system migration logic

## Caller Ownership

Existing owner classes remain responsible for renderer meaning:

| owner | remains responsible for |
|---|---|
| `LLRenderTarget` | framebuffer lifetime, attachments, clear intent, viewport intent |
| `LLImageGL` | texture names, upload policy, mipmap policy, readback/copy intent |
| `LLVertexBuffer` | buffer lifetime, vertex declaration, draw/index intent |
| `LLGLSLShader` / shader managers | shader/program lifetime, uniforms, validation |
| draw pools and `pipeline.cpp` | frame order, render passes, draw ordering |
| UI/viewer files | whether a UI operation needs renderer work |

The containment layer should not make decisions for these owners. It should
only expose the already-decided operation through a non-raw OpenGL function
name.

## Adding A Wrapper

Only add a wrapper when all of the following are true:

- a specific raw runtime `gl*` callsite exists
- the current owner and ordering are understood
- the wrapper can preserve behavior exactly
- the wrapper belongs at the `indra/llrender` OpenGL boundary
- the change can be reviewed as a small packet

Each new wrapper should:

- use a name that describes intent from the caller perspective
- keep parameters close to the existing callsite data
- avoid bundling unrelated GL calls
- avoid state save/restore unless the caller already performed it
- keep platform exclusions explicit with the same constraints as the original
  callsite

## Forbidden Patterns

Do not introduce:

- generic `callGL(...)`, `withGL(...)`, or lambda dispatch wrappers
- broad scoped state guards without a documented leak pattern
- wrappers that allocate or delete resources outside the original owner
- wrappers that reorder several GL operations to look cleaner
- wrappers whose only purpose is to hide complicated caller ownership
- new direct runtime `gl*` calls outside `llglcontainment.cpp`

## Exceptions

Allowed direct runtime calls:

- `indra/llrender/llglcontainment.cpp`

Allowed non-runtime GL references:

- `indra/llrender/llglheaders.h`: GL ABI declarations
- `indra/llrender/llgl.cpp`: loader names and symbol-loading references
- generated inventory references and documentation

If a true runtime `glFoo(...)` call appears anywhere else, it should fail the
containment guardrail and be routed through `LLGLContainment` or justified in a
focused follow-up.

## Required Checks

Before committing future containment changes:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

For source changes, also run the narrowest useful build target. For
`llrender`-only edits, prefer `llrender/fast` before any viewer-level build.
