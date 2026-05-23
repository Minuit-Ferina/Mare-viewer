# GLTF Preview State Summary

Date: 2026-05-23

Branch: `phase5`

## Scope

This packet follows `docs/architecture/263-gltf-preview-state-task.md`.

The source change is owner-local to
`indra/newview/llgltfmaterialpreviewmgr.cpp`.

## Source Changes

Added anonymous-namespace `GLTFPreviewRenderState`.

It now owns the temporary preview render state that was previously held by
separate locals in `LLGLTFPreviewTexture::render()`:

- disabled depth test;
- disabled stencil test;
- disabled scissor test;
- temporary `LLPipeline::RenderDepthOfField = false`;
- temporary `LLPipeline::sRenderGlow = false`;
- temporary `LLPipeline::RenderScreenSpaceReflections = false`;
- temporary `LLPipeline::RenderFSAAType = 0`;
- temporary `gPipeline.mRT = &gPipeline.mAuxillaryRT`;
- saved and temporary `RenderLocalLightCount = 0`;
- forced preview reflection-probe state.

`render()` still calls `preview_state.restore()` at the original cleanup point.
The destructor is a fallback and does nothing after explicit restore.

This preserves the previous ordering:

1. set temporary GL and pipeline state;
2. render preview;
3. restore lights, reflection probe state, and local light count;
4. let the temporary GL and pipeline state helpers destruct at function exit.

## Behavior Preserved

This packet does not change:

- material load policy;
- UI entry points;
- shader selection;
- shader constants;
- preview sphere geometry;
- post-processing order;
- render target selection;
- `mBoundTarget` usage;
- dynamic texture lifecycle.

## Risk

Risk is low-medium.

Why:

- it changes the lifetime expression for sensitive render state;
- explicit `restore()` keeps the previous cleanup point;
- the owner, target, shader, and post-processing behavior are unchanged.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llgltfmaterialpreviewmgr.cpp.o -j8
```

Result: passed.

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- source inventory regenerated; scanned 3085 source files;
- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers do not include `llgl.h` directly outside PCH/prefix;
- raw GL scalar type names are confined to `llglheaders.h`;
- whitespace check passed.

## Integration Build

No non-clean `mare-viewer` integration checkpoint was run for this packet.

Reason:

- targeted owner build passed;
- OpenGL guardrails passed;
- this packet only groups local state lifetime in the same owner.

## Next Small Tasks

- Move to a new phase 5 owner map.
- Recommended next owner: `LLViewerTexLayerSetBuffer`, because it is the other
  high-risk `LLViewerDynamicTexture` branch and covers avatar bake state rather
  than UI material preview state.
