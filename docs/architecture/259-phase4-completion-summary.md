# Phase 4 Completion Summary

Date: 2026-05-23

Branch: `phase4`

Base branch: `phase3`

## Result

Phase 4 is complete.

The phase met its exit criteria:

- a renderer-boundary owner was mapped before source edits;
- source changes stayed owner-local and behavior-preserving;
- OpenGL containment guardrails stayed green;
- targeted builds passed;
- one non-clean integration `mare-viewer` build reached
  `[100%] Built target mare-viewer`.

## Completed Source Owners

### `LLDrawPoolAlpha`

Phase 4 first mapped and then reorganized `LLDrawPoolAlpha` into smaller
owner-local helpers.

Main areas grouped:

- GLTF alpha shader target selection;
- non-GLTF alpha shader setup;
- per-draw alpha state and draw calls;
- alpha emissive subpass and emissive draw helpers;
- alpha traversal setup and final restore;
- texture setup;
- post-deferred alpha setup and depth-of-field alpha pass;
- spatial group rendering;
- forward alpha setup/finish;
- debug static/rigged alpha rendering;
- owner-local `AlphaRenderState`;
- owner-local `AlphaPassContext`.

Behavior policy was not changed:

- draw-pool pass order stayed the same;
- shader/material/PBR policy stayed the same;
- alpha render target usage stayed the same;
- runtime smoke was deferred or handled separately per packet.

### `LLViewerDynamicTexture`

Phase 4 then opened the next UI/render boundary and applied one final
owner-local source packet:

- added `docs/architecture/257-dynamic-texture-ui-preview-owner-map.md`;
- split preview-target and bake-target orchestration into private
  `LLViewerDynamicTexture` helpers;
- preserved update order, render target selection, clears, flushes, shader
  unbind, vertex-buffer unbind, viewport/camera behavior, and return semantics.

## Validation Summary

Guardrails passed after the final source packet:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Final source inventory:

- scanned 3085 source files;
- regenerated `docs/architecture/generated/source_inventory.csv`;
- `source_inventory_top.md` did not need content changes.

Final targeted build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o -j8
```

Result: passed.

Final non-clean integration checkpoint:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 mare-viewer -j8
```

Result: passed with `[100%] Built target mare-viewer`.

## Known Build Warnings

Known warnings remain outside this phase 4 source scope:

- deprecated enum arithmetic in `indra/newview/gltf/llgltfloader.cpp`;
- deprecated enum arithmetic in `indra/newview/llpaneloutfitedit.cpp`;
- `RAND_MAX` integer-to-float conversion warnings in
  `indra/newview/llvoicewebrtc.cpp`;
- linker platform-load-command warnings from prebuilt `libjpeg.a` objects.

These are not caused by the final phase 4 packet.

## What Phase 4 Did Not Do

Phase 4 did not:

- port OpenGL to Vulkan;
- introduce SDL;
- move source files;
- change viewer lifecycle;
- change multi-window or multi-login behavior;
- rewrite renderer architecture;
- change draw-pool pass ordering;
- change alpha material/PBR policy;
- change dynamic texture subclass behavior.

## Recommended Phase 5 Direction

Start phase 5 with a new task-specific document before source edits.

Recommended candidate:

- UI/render split around dynamic preview users, starting with either
  `LLViewerTexLayerSetBuffer` or `LLGLTFPreviewTexture`.

Alternative candidates:

- map UI rendering around `LLNetMap` and `LLWorldMapView`;
- core `llui` clipping and immediate draw ownership;
- shader manager lifecycle and shader-family ownership;
- build-system guardrails for Makefile/Xcode divergence.

Do not treat phase 5 as a backend port. The useful next step is still
containment by owner and contract, not Vulkan or Metal work.
