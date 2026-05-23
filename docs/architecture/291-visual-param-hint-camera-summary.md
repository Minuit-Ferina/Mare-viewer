# Visual Param Hint Camera Summary

Date: 2026-05-23

Branch: `phase6`

## Scope

This packet follows
`docs/architecture/290-visual-param-hint-camera-task.md`.

The source change is limited to the appearance-editor preview owner:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Source Changes

Split pure camera math out of `LLVisualParamHint::render()` into private
owner-local helpers:

- `getAvatarRenderRotation() const`
- `getCameraTargetPosition(const LLQuaternion& avatar_rotation,
  const LLVector3& target_joint_pos) const`
- `getCameraPosition(const LLQuaternion& avatar_rotation,
  const LLVector3& target_joint_pos) const`

`render()` still owns camera application:

- `gGL.flush()`;
- `LLViewerCamera::setAspect(...)`;
- `LLViewerCamera::setOriginAndLookAt(...)`;
- `LLViewerCamera::setPerspective(...)`.

## Behavior Preserved

This packet does not change:

- avatar root rotation fallback;
- target joint world-position read timing;
- visual-param camera elevation use;
- visual-param camera angle use;
- `AppearanceCameraMovement` policy;
- `F_PI` angle adjustment when automatic camera movement is disabled;
- visual-param camera distance use;
- camera aspect/perspective setup;
- matrix push/pop ordering;
- background draw timing;
- impostor generation;
- visual-param restore ordering.

## Risk

Risk is low to medium.

Why:

- no `gGL` calls were moved;
- no camera application calls were moved;
- the packet only names the existing camera math inputs and formulas.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8
```

Result: passed.

Notes:

- the targeted build rebuilt the `mare-viewer` PCH objects for `x86_64` and
  `arm64` before `lltoolmorph.cpp.o`;
- no clean or full app build was run for this packet.

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

## Next Small Tasks

- Decide whether to split `LLVisualParamHint::render()` background matrix work
  into a scoped helper.
- Do not change `gPipeline.generateImpostor(...)` ordering before a dedicated
  task names blend, depth, and visual-param restore behavior.
