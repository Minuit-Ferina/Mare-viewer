# Model Preview Camera Task

Date: 2026-05-24

Branch: `phase10`

## Task

Split the 3D preview camera, shader bind, and preview-light setup from
`LLModelPreview::render()` into a private owner-local helper without changing
render order or runtime behavior.

## Source Scope

Allowed files:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

No other source files should change in this packet.

## Helper Boundary

Add private owner state:

- `PreviewCameraState`

Add private helper:

- `setupPreviewCamera(bool show_skin_weight, S32 width, S32 height)`

The helper should only name existing work:

- preview panel aspect setup;
- viewer camera FOV, origin, look-at, near/far, and perspective setup;
- skinned preview target switch and refresh;
- `gObjectPreviewProgram` bind;
- modelview identity load;
- preview light enablement.

## Required Ordering

Preserve this order:

1. read `physics_explode`;
2. create the existing `LLGLDepthTest gls_depth(GL_TRUE)` scope;
3. configure preview camera, shader, and lights;
4. push the model matrix;
5. set preview edge color;
6. continue existing buffer generation and draw paths;
7. keep the skinned preview camera recentering before joint override work.

## Explicit Non-Goals

Do not change:

- camera math;
- skinned avatar recentering behavior;
- `refresh()` timing for skinned previews;
- shader choices;
- model, physics, skinned avatar, joint, or debug drawing;
- dynamic texture order or target behavior;
- `pipeline.cpp` or broad `llui`.

## Verification

Targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8
```

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```
