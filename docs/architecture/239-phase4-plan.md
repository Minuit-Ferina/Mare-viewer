# Phase 4 Plan

Date: 2026-05-23

Branch: `phase4`

Base branch: `phase3`

## Purpose

Phase 4 starts after the phase 3 OpenGL containment closure.

The goal is not to add more broad wrappers. The goal is to use the containment
boundary as a stable guardrail while preparing the next renderer boundaries in
small, reviewable steps.

## Phase 3 Baseline

Phase 3 ended with these guardrails passing:

- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers do not include `llgl.h` directly outside PCH/prefix;
- raw GL scalar type names are confined to `llglheaders.h`;
- non-clean Makefile `mare-viewer` checkpoint reached
  `[100%] Built target mare-viewer`;
- user runtime run after phase 3 closure worked.

Phase 4 must preserve that baseline.

## Allowed Work

Allowed phase 4 work:

- renderer-boundary maps under `docs/architecture/`;
- small task notes before source changes;
- local helper extraction only when it preserves runtime behavior;
- explicit implementation dependencies needed to keep builds healthy;
- guardrail/tooling refinements that make future renderer work safer;
- targeted and non-clean integration builds at meaningful checkpoints.

## Not Allowed

Do not do these in phase 4:

- start a Vulkan backend;
- port the viewer to SDL;
- move source files;
- perform a renderer rewrite;
- change draw-pool pass ordering;
- change shader selection behavior without a dedicated task and runtime plan;
- change material/PBR alpha policy without a dedicated task and runtime plan;
- change app lifecycle, multi-window, or multi-login behavior;
- do signing, notarization, DMG, or release packaging.

## First Candidate

Recommended first candidate:

- `LLDrawPoolAlpha` normal alpha shader-selection map.

Reason:

- phase 3 already cleaned up surrounding `LLDrawPoolAlpha` decisions;
- the code is now easier to inspect;
- the next sensitive area is shader/material/PBR selection, which should be
  documented before any source movement;
- the last phase 3 checkpoint intentionally stopped before this area.

Initial work should be docs-only:

- map all shader-selection branches in `renderAlpha(...)`;
- separate GLTF blend, material, fullbright, HUD, rigged, and exposure-map
  cases;
- list invariants that must not change;
- list the targeted object build and runtime scenes needed before any source
  patch.

## Other Candidate Areas

Other possible phase 4 candidates:

- dynamic texture ownership contracts after the private helper extraction;
- map/world UI rendering boundaries after `lllocalcliprect` cleanup;
- shader manager lifecycle status after phase 3 containment;
- draw-pool pass-state contracts beyond alpha;
- build guardrail cleanup for known Makefile/Xcode divergence.

## Verification

For docs-only packets:

```sh
git diff --check
```

For source packets:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Also run a targeted build for the touched owner. For `LLDrawPoolAlpha`:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lldrawpoolalpha.cpp.o -j8
```

Run a non-clean `mare-viewer` checkpoint before declaring a phase 4 source
milestone complete.

## Exit Criteria

Phase 4 can stop after one renderer-boundary owner is mapped and, if safe, one
small behavior-preserving source packet is validated.

A phase 4 packet is reviewable when:

- the task-specific note exists before source edits;
- runtime behavior is explicitly preserved;
- source changes are owner-local;
- guardrails pass;
- targeted build checks pass;
- runtime smoke is either completed or explicitly deferred with a reason.
