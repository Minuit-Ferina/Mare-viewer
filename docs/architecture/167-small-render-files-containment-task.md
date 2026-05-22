# Small Render Files Containment Task

Branch: `phase3`
Source owners: small render/UI/viewer files

## Scope

Route direct OpenGL calls in small files through `llglcontainment.*`.

Included source files:
- `indra/llappearance/lltexlayer.cpp`
- `indra/llui/lllocalcliprect.cpp`
- `indra/newview/RRInterface.cpp`
- `indra/newview/lldrawpool.cpp`
- `indra/newview/lldrawpoolbump.cpp`
- `indra/newview/lldrawpoolsimple.cpp`
- `indra/newview/lldrawpooltree.cpp`
- `indra/newview/lldrawpoolwlsky.cpp`
- `indra/newview/lldynamictexture.cpp`
- `indra/newview/llfasttimerview.cpp`
- `indra/newview/llfloaterimagepreview.cpp`
- `indra/newview/llgltfmaterialpreviewmgr.cpp`
- `indra/newview/llheroprobemanager.cpp`
- `indra/newview/llhudeffectlookat.cpp`
- `indra/newview/llhudeffectpointat.cpp`
- `indra/newview/llmanipscale.cpp`
- `indra/newview/llnetmap.cpp`
- `indra/newview/llsnapshotlivepreview.cpp`
- `indra/newview/llterrainpaintmap.cpp`
- `indra/newview/llviewercamera.cpp`
- `indra/newview/llviewerjoint.cpp`
- `indra/newview/llviewerparceloverlay.cpp`
- `indra/newview/rlveffects.cpp`

Included raw OpenGL families:
- `glClear`
- `glClearColor`
- `glViewport`
- `glLineWidth`
- `glGetFloatv`
- `glReadPixels`
- `glGetTexImage`
- `glGetError`
- `glGenerateMipmap`
- `glCopyTexSubImage3D`
- `glPolygonOffset`
- `glCullFace`
- `glMatrixMode`
- `glPushMatrix`
- `glPopMatrix`
- `glScissor`
- `glColor4ubv`

## Non-Scope

Do not change:
- render order
- snapshot/export dimensions or formats
- texture copy offsets
- mipmap targets
- cull orientation
- polygon offset values
- matrix stack ordering
- scissor rectangle math
- UI/debug visibility rules

Do not include:
- `pipeline.cpp`
- FSR2/TA upscalers
- platform windowing GL glue
- `llcommon` headers
- broad low-level `llrender`/`llgl` ownership files

## Ownership Notes

Each source file keeps ownership of:
- its render decision and ordering
- its geometry, target, and readback dimensions
- its debug/UI visibility behavior

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: low to medium.

Why:
- This is wrapper-only.
- The batch touches many files, so verification must compile the affected
  libraries/objects instead of relying only on review.
- Snapshot/readback callsites are user-visible but keep the exact same
  dimensions, formats, and buffers.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- `llappearance/fast`
- `llui/fast`
- targeted newview object builds for affected files
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; later checks should include scene render, minimap,
  snapshot/export paths, and UI clipping.
