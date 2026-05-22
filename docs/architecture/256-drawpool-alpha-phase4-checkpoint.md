# Draw Pool Alpha Phase 4 Checkpoint

Date: 2026-05-23

## Scope

This checkpoint summarizes the phase 4 `LLDrawPoolAlpha` cleanup block after
the GLTF shader helper baseline.

Changed owner files:

- `indra/newview/lldrawpoolalpha.h`
- `indra/newview/lldrawpoolalpha.cpp`

Generated inventory refreshed throughout:

- `docs/architecture/generated/source_inventory.csv`
- `docs/architecture/generated/source_inventory_top.md`

## Source Packets

Included packets:

- `render: group non-gltf alpha shader setup`
- `render: group alpha draw state`
- `render: group alpha emissive subpass`
- `render: group alpha traversal setup`
- `render: group alpha texture setup`
- `render: group alpha post deferred setup`
- `render: group alpha emissive draws`
- `render: group alpha draw pipeline`
- `render: group alpha spatial group pipeline`
- `render: group alpha forward render`
- `render: group alpha debug rendering`
- `render: group alpha pass state`
- `render: group alpha pass context`

## Boundary Now Visible

`LLDrawPoolAlpha` now has named owner-local responsibilities for:

- deferred alpha shader preparation;
- forward alpha render setup and finish;
- alpha traversal context;
- alpha render pass state;
- spatial group traversal;
- per-draw alpha pipeline;
- GLTF and non-GLTF shader selection;
- legacy and GLTF texture setup;
- alpha draw blend/minimum-alpha state;
- emissive queueing and emissive subpass rendering;
- debug alpha static and rigged rendering.

## Behavior Preserved

Preserved:

- shader preparation order;
- `LLGLSPipelineAlpha` and `LLGLDepthTest` lifetimes;
- rigged GLTF depth prepass condition;
- GLTF material bind after shader bind;
- non-GLTF material/fullbright/HUD shader selection;
- matrix-palette upload order;
- `TexSetup(...)`, draw, emissive queue, and `RestoreTexSetup(...)` order;
- emissive queue membership and render order;
- debug alpha colors and batch IDs;
- final scene blend, vertex-buffer unbind, and dynamic-light restore.

## Verification

Each source packet was verified with:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results across the block:

- targeted `lldrawpoolalpha.cpp.o`: passed after each packet;
- generated source inventory: refreshed after each packet;
- GL containment guardrail: passed after each packet;
- header boundary guardrail: passed after each packet;
- diff whitespace check: passed after each packet.

Runtime smoke:

- Not run for this block. The packets are behavior-preserving helper and
  state/context extractions. A runtime checkpoint is still appropriate before
  closing phase 4.

## Integration Checkpoint

Command run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 mare-viewer -j8
```

Result:

- passed;
- final output reached `[100%] Built target mare-viewer`;
- app bundle staging copied OpenAL, ALUT, WebRTC, SLPlugin, CEF, VLC, shader,
  skin, font, character, and settings resources.

Notes:

- This was a non-clean Makefile integration checkpoint.
- The build still rebuilt a large part of `newview`.
- Existing warnings were observed outside the `LLDrawPoolAlpha` packet:
  - enum arithmetic warning in `gltf/llgltfloader.cpp`;
  - enum arithmetic warning in `llpaneloutfitedit.cpp`;
  - `RAND_MAX` integer-to-float conversion warnings in `llvoicewebrtc.cpp`;
  - linker warnings for prebuilt `libjpeg.a` object platform load commands.

## Remaining Near-Term Work

Recommended next steps:

- perform a runtime smoke when convenient: login, scene load, resize, alpha
  content, and debug alpha only if that mode is available;
- then either close phase 4 with a review summary or start the next owner with
  a docs-only map.
