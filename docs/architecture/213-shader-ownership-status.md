# Shader Ownership Status

Date: 2026-05-22

## Scope

This note records that shader manager ownership has been mapped enough to
satisfy the initial "do not edit shader managers until ownership is mapped"
guardrail.

It does not make shader changes low-risk.

## Current Layers

| layer | primary files | owner responsibility |
|---|---|---|
| Program wrapper | `indra/llrender/llglslshader.*` | shader program objects, file lists, defines, uniforms, texture channels, GLTF variants, bind/unbind behavior |
| Compile/link manager | `indra/llrender/llshadermgr.*` | shader file loading, GLSL source generation, compile/link/validate policy, reserved uniforms/attributes, shader binary cache |
| Viewer shader registry | `indra/newview/llviewershadermgr.*` | global shader handles, feature flags, shader group loading, GLTF/rigged variants, viewer uniform propagation |
| Shader assets | `indra/newview/app_settings/shaders/` | GLSL source families and class-level fallback content |

Primary ownership map:

- `docs/architecture/06-shader-map.md`

## Current Inventory Snapshot

From the regenerated source inventory:

| file | `gl_calls` | `LLGL` | `LLPipeline` | note |
|---|---:|---:|---:|---|
| `indra/llrender/llglslshader.cpp` | 0 | 292 | 0 | program wrapper; direct GL calls contained |
| `indra/llrender/llshadermgr.cpp` | 0 | 60 | 0 | compile/link manager; direct GL calls contained |
| `indra/newview/llviewershadermgr.cpp` | 0 | 214 | 7 | viewer registry and load orchestration |
| `indra/llrender/llglslshader.h` | 0 | 92 | 0 | public shader program interface |
| `indra/llrender/llshadermgr.h` | 0 | 12 | 0 | compile/link manager interface |
| `indra/newview/llviewershadermgr.h` | 0 | 135 | 0 | viewer shader globals and registry declarations |

## Containment Status

Relevant completion docs:

- `docs/architecture/109-llglslshader-call-family-map.md`
- `docs/architecture/111-llglslshader-profile-query-summary.md`
- `docs/architecture/113-llglslshader-metadata-query-summary.md`
- `docs/architecture/115-llglslshader-texture-channel-uniform-summary.md`
- `docs/architecture/117-llglslshader-public-uniform-summary.md`
- `docs/architecture/119-llglslshader-vertex-attrib-summary.md`
- `docs/architecture/122-llglslshader-ubo-binding-summary.md`
- `docs/architecture/124-llglslshader-attribute-binding-summary.md`
- `docs/architecture/126-llglslshader-program-binding-summary.md`
- `docs/architecture/128-llglslshader-create-attach-summary.md`
- `docs/architecture/130-llglslshader-unload-lifecycle-summary.md`
- `docs/architecture/132-llglslshader-debug-include-summary.md`
- `docs/architecture/134-llshadermgr-containment-summary.md`
- `docs/architecture/189-llglslshader-header-boundary-summary.md`
- `docs/architecture/190-llshadermgr-header-boundary-summary.md`

Current state:

- Direct runtime `gl*` calls in `llglslshader.cpp` are contained.
- Direct runtime `gl*` calls in `llshadermgr.cpp` are contained.
- Header-level GL exposure has been narrowed.
- `LLViewerShaderMgr` remains the high-level viewer shader registry and should
  not be treated as a low-level OpenGL call owner.

## Guardrail

Shader managers may now be analyzed or changed only through a focused task.

A future shader task must state:

- which layer owns the change;
- which shader group or asset family is affected;
- whether feature flags or generated defines change;
- whether GLTF variants are affected;
- whether shader binary cache behavior changes;
- targeted build path;
- startup/login validation plan if compile/link behavior changes.

Do not combine shader manager changes with draw-pool, pipeline, FSR2, or UI
rendering behavior changes in the same packet.
