# Phase 6 Completion Summary

Date: 2026-05-23

Branch: `phase6`

## Scope Completed

Phase 6 stayed focused on appearance-editor visual-parameter preview dynamic
textures in:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

The phase mapped both owners:

- `LLVisualParamHint`
- `LLVisualParamReset`

Source cleanup was limited to behavior-preserving helper extraction inside
`LLVisualParamHint`.

## Source Packets

Committed phase 6 source packets:

- `4151ae7328 render: start phase6 visual param hints`
- `8ea651f6e0 render: split visual param hint prerender`
- `0aa3c7415d render: split visual param hint camera math`
- `ec9aec4670 render: split visual param hint background draw`
- `7ebc819000 render: split visual param hint impostor`
- `725da56d93 render: split visual param hint finalize`

The resulting owner-local helpers name:

- render update gates;
- preview wearable/avatar state setup;
- preview avatar geometry refresh;
- camera target/origin math;
- hint background draw;
- avatar impostor generation;
- preview state restore;
- final dynamic texture marking.

## Verification

Per-packet verification passed for each source packet:

- targeted `lltoolmorph.cpp.o` build;
- regenerated source inventory;
- `python3 tools/architecture/check_gl_containment.py .`;
- `python3 tools/architecture/check_gl_header_boundaries.py .`;
- `git diff --check`.

Final non-clean integration checkpoint passed:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 mare-viewer -j8
```

Result:

```text
[100%] Built target mare-viewer
```

Known warnings still present during the integration build:

- deprecated enum arithmetic in `indra/newview/gltf/llgltfloader.cpp`;
- deprecated enum arithmetic in `indra/newview/llpaneloutfitedit.cpp`;
- `RAND_MAX` integer-to-`F32` conversion warnings in
  `indra/newview/llvoicewebrtc.cpp`;
- linker platform-load warnings from packaged `libjpeg.a` objects.

These warnings were already observed outside phase 6 and are not caused by the
`LLVisualParamHint` helper split.

## Behavior

No runtime behavior is intentionally changed.

The phase preserves:

- visual-param update gating and delay decrement order;
- wearable volatile lifetime;
- preview visual-param mutation and restore order;
- blink suppression timing;
- avatar composite, visual-param, geometry, and LOD update timing;
- UI matrix and projection/modelview push/pop order;
- background draw state;
- camera setup before impostor generation;
- depth/blend state around impostor generation;
- final color restore and GL texture-created marking.

## Residual Coupling

Remaining coupling is documented and intentionally not changed in this phase:

- `LLVisualParamHint::render()` still owns the outer UI matrix lifetime;
- dynamic texture render target and camera save/restore still live in the base
  dynamic texture flow;
- `LLVisualParamReset::render()` remains mapped but untouched;
- appearance preview rendering still coordinates avatar state, UI drawing, and
  impostor generation in one owner file.

## Suggested Next Phase

Start phase 7 with a docs-first plan before touching source.

Reasonable candidates:

- map and split `LLVisualParamReset::render()` only if there is useful owner
  separation left there;
- map `LLVisualParamHint::draw(...)` as the next UI-facing draw path;
- move to another narrow `LLViewerDynamicTexture` preview owner rather than
  broad `llui` or `pipeline` work.
