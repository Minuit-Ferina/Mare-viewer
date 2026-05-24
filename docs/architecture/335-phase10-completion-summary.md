# Phase 10 Completion Summary

Date: 2026-05-24

Branch: `phase10`

## Scope

Phase 10 focused on the model upload preview owner:

- `indra/newview/llmodelpreview.h`
- `indra/newview/llmodelpreview.cpp`

The phase kept behavior unchanged and split `LLModelPreview::render()` into
owner-local helpers so the remaining UI/render coupling is easier to inspect.

## Source Packets

Commits in this phase:

- `9af07f3817 render: start phase10 model preview`
- `2df20b3b84 render: split model preview prep`
- `b95e6c6ab8 render: split model preview camera setup`
- `d3d63538a4 render: split model preview nonskinned draw`
- `ac1413272e render: split model preview physics draw`
- `25db29ba3f render: split model preview skinned draw`

`LLModelPreview::render()` now delegates these render sub-blocks:

- preview canvas background drawing;
- skin/upload control preparation;
- preview LOD vertex-buffer preparation;
- preview material application;
- camera setup;
- non-skinned model drawing;
- physics preview drawing;
- skinned avatar preview drawing.

## Behavior Notes

No runtime behavior is intentionally changed.

This phase does not change:

- model parsing, upload, validation, or LOD policy;
- physics preview semantics;
- skinned preview joint override, pelvis fixup, or matrix palette order;
- dynamic texture ordering;
- shader selection;
- material binding semantics;
- app/session lifecycle;
- SDL, Vulkan, Metal, multi-window, or multi-login behavior.

No `needsRender()` override was added. That remains behavior work, not a
wrapper-only cleanup task.

## Verification

Each source packet passed:

- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llmodelpreview.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`
- `python3 tools/architecture/check_gl_containment.py .`
- `python3 tools/architecture/check_gl_header_boundaries.py .`
- `git diff --check`

The final non-clean integration checkpoint also passed:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 mare-viewer -j8
```

Result:

```text
[100%] Built target mare-viewer
```

The manifest copy included the previously fragile macOS runtime libraries:

- `Contents/Frameworks/libopenal.dylib`
- `Contents/Frameworks/libalut.dylib`
- `Contents/Frameworks/libllwebrtc.dylib`

Known existing warnings seen during the integration checkpoint:

- `llgltfloader.cpp`: deprecated arithmetic between different enum types;
- `llpaneloutfitedit.cpp`: deprecated arithmetic between different enum types;
- `llvoicewebrtc.cpp`: `RAND_MAX` integer-to-`F32` conversion warnings;
- linker warnings for older `libjpeg.a` objects without a platform load
  command.

These warnings are not introduced by phase 10.

## Residual Risk

`LLModelPreview` is more readable, but it is still a coupled preview owner:

- UI mutation still happens from the render path;
- preview vertex-buffer generation can still happen during render;
- the model upload floater and preview renderer remain tightly coupled;
- physics and skinned-avatar preview state still share one owner;
- all helpers remain owner-local and still run in the original call order.

## Next Step

Do not continue with another helper-only phase by default.

The next useful phase should map and then implement a behavior-preserving
separation of `LLModelPreview` UI mutation from render work. Any source packet
should name the exact state ownership, call order, risk, and verification plan
before edits.
