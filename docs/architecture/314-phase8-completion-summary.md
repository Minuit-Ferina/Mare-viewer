# Phase 8 Completion Summary

Date: 2026-05-24

Branch: `phase8`

## Scope Completed

Phase 8 stayed focused on image upload preview dynamic texture owners in:

- `indra/newview/llfloaterimagepreview.h`
- `indra/newview/llfloaterimagepreview.cpp`

The phase split two narrow owners:

- `LLImagePreviewAvatar::render()`
- `LLImagePreviewSculpted::render()`

The phase did not move files, did not change runtime behavior intentionally,
and did not touch broad `llui`, `pipeline`, app lifecycle, SDL, Vulkan, Metal,
multi-window, or multi-login work.

## Source Packets

Committed phase 8 source packets:

- `2b3f161752 render: start phase8 image preview avatar`
- `c3139c98db render: split image preview sculpted`

The resulting owner-local helpers name:

- preview background drawing;
- preview camera setup;
- avatar preview draw ownership;
- sculpted preview volume draw ownership.

## Verification

Per-packet verification passed:

- targeted `llfloaterimagepreview.cpp.o` build;
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

The final package copy included the expected OpenAL runtime library:

```text
Processing libopenal.dylib => Contents/Frameworks/libopenal.dylib ... 1 files
```

Known warnings still present during the integration build:

- deprecated enum arithmetic in `indra/newview/gltf/llgltfloader.cpp`;
- deprecated enum arithmetic in `indra/newview/llpaneloutfitedit.cpp`;
- `RAND_MAX` integer-to-`F32` conversion warnings in
  `indra/newview/llvoicewebrtc.cpp`;
- linker platform-load warnings from packaged `libjpeg.a` objects.

These warnings were already observed outside phase 8 and are not caused by the
`llfloaterimagepreview` helper splits.

## Behavior

No runtime behavior is intentionally changed.

The phase preserves:

- `mNeedsUpdate` clear timing in both preview owners;
- UI/default GL state scopes around preview background drawing;
- projection/modelview push/pop ordering;
- avatar preview camera setup and pelvis rotation ordering;
- avatar LOD update, drawable face checks, and avatar pool draw path;
- sculpted preview depth clear timing;
- sculpted preview shader bind/draw/unbind ordering;
- sculpted preview vertex-buffer draw mode, offset, and index count.

## Residual Coupling

Remaining coupling is documented and intentionally not changed in this phase:

- image upload preview UI and preview rendering still live in the same floater
  implementation file;
- `LLImagePreviewAvatar` still coordinates dummy avatar state, lighting,
  camera, and preview drawing;
- `LLImagePreviewSculpted` still coordinates preview camera, shader, lighting,
  vertex buffer, and volume draw state;
- broad UI rendering ownership remains outside this phase.

## Suggested Next Phase

Start phase 9 with a docs-first plan before touching source.

Reasonable candidates:

- map `LLPreviewAnimation` or another narrow dynamic texture preview owner;
- compare the now-split upload preview owners against other preview classes;
- defer `LLModelPreview` until a dedicated map names render, upload, material,
  camera, and LOD risks;
- continue avoiding broad `llui` and `pipeline` work until a specific owner map
  names exact callsites and ordering risks.
