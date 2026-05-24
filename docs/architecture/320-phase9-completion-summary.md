# Phase 9 Completion Summary

Date: 2026-05-24

Branch: `phase9`

## Scope Completed

Phase 9 stayed focused on the BVH animation preview dynamic texture owner in:

- `indra/newview/llfloaterbvhpreview.h`
- `indra/newview/llfloaterbvhpreview.cpp`

The phase split:

- `LLPreviewAnimation::render()`

It also documented the existing `needsUpdate()` versus `needsRender()` refresh
semantics without changing them.

The phase did not move files, did not change runtime behavior intentionally,
and did not touch broad `llui`, `pipeline`, app lifecycle, SDL, Vulkan, Metal,
multi-window, or multi-login work.

## Commits

Committed phase 9 packets:

- `ac6b01be6f render: start phase9 preview animation`
- `258ff73452 docs: map preview animation refresh`

The resulting owner-local helpers name:

- preview background drawing;
- preview camera setup;
- preview avatar draw ownership.

## Verification

Per-packet verification passed:

- targeted `llfloaterbvhpreview.cpp.o` build;
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

These warnings were already observed outside phase 9 and are not caused by the
`LLPreviewAnimation` helper split.

## Behavior

No runtime behavior is intentionally changed.

The phase preserves:

- `mNeedsUpdate` clear timing in `LLPreviewAnimation::render()`;
- projection/modelview push/pop ordering;
- `gUIProgram` bind before the `LLGLSUIDefault` scope;
- preview background color and texture-unit unbind;
- camera target, rotation, FOV, aspect, and perspective setup;
- avatar drawable, face, LOD, dirty mesh, lighting, and avatar pool draw order;
- final immediate color reset;
- the existing `needsUpdate()` method and lack of `needsRender()` override.

## Residual Coupling

Remaining coupling is documented and intentionally not changed in this phase:

- `LLPreviewAnimation` is shared by the BVH upload floater and animation
  explorer;
- visible UI owners request preview updates from their draw paths;
- `needsUpdate()` is not the virtual used by `LLViewerDynamicTexture`;
- changing this to a `needsRender()` override would be a behavior task, not a
  wrapper-only cleanup.

## Suggested Next Phase

Start phase 10 with a docs-first plan before touching source.

Reasonable candidates:

- map `LLGLTFPreviewTexture` before material preview source cleanup;
- map `LLModelPreview` before deciding whether any source work is safe there;
- investigate `LLPreviewAnimation::needsRender()` as an explicit behavior task
  with runtime checks;
- continue avoiding broad `llui` and `pipeline` work until a specific owner map
  names exact callsites and ordering risks.
