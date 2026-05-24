# Phase 12 Completion Summary

Date: 2026-05-24

Branch: `phase12`

## Scope

Phase 12 continued the model preview UI/render ownership work started in
phase 10 and phase 11, focused on `LLModelPreview` and
`LLFloaterModelPreview`.

The branch moved many direct floater control reads and mutations out of
`LLModelPreview` and into `LLFloaterModelPreview` owner methods without
intended runtime behavior changes.

## Completed Ownership Moves

Completed families:

- preview texture size and render-target sizing ownership;
- dimension and import-scale option reads;
- upload status and upload-data option reads;
- crease-angle reads and control synchronization;
- LOD optimizer option reads;
- upload-data description and import-scale field synchronization;
- default description synchronization after load;
- calculate and upload button enablement;
- LOD control visibility, generated-control values, and selected-row sync;
- physics file controls;
- show-physics option sync;
- physics decomposition controls;
- physics summary text.

## Verification Policy Used

Each source packet used targeted validation:

- `llmodelpreview.cpp.o`;
- `llfloatermodelpreview.cpp.o`;
- regenerated source inventory;
- OpenGL containment guardrail;
- GL header boundary guardrail;
- `git diff --check`.

No broad `mare-viewer` integration build was run during these packets.

## Result

`LLModelPreview` is now a useful prototype for the UI/preview ownership split:

- `LLModelPreview` increasingly owns preview/model/render decisions;
- `LLFloaterModelPreview` increasingly owns floater control reads and
  mutations;
- behavior was kept stable through small, reviewable commits.

## Strategy Change

Do not continue phase 13 by fully exhausting `LLModelPreview` line by line.

That would not scale across the viewer. The better next step is breadth-first:

- identify repeated UI/preview/render patterns across multiple preview files;
- apply small helper moves across several files in one coherent packet;
- keep per-packet validation targeted to the affected object files;
- avoid broad rebuilds unless a branch checkpoint explicitly needs one.

## Remaining `LLModelPreview` Risk

Remaining UI access in `LLModelPreview` includes:

- LOD and physics status text/icon updates;
- initial rig option mutations during model-load callback;
- preview panel rectangle access used for rendering.

These can be addressed later, but they should not block moving to multi-file
coverage.

## Phase 13 Direction

Phase 13 should focus on preview/UI-render hotspots across files, not a single
deep owner:

- `llfloaterimagepreview.*`;
- `llpreviewtexture.*`;
- `llviewertexlayer.*`;
- `llvisualparamhint.*`;
- `lltexturectrl.*` and related preview controls if they match the pattern;
- other inventory-ranked preview widgets with mixed UI/render access.
