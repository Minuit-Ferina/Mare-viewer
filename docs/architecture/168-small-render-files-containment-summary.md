# Small Render Files Containment Summary

Branch: `phase3`
Source commit: `c90a71d859`

## Scope Completed

Routed direct OpenGL calls in small render/UI/viewer files through
`llglcontainment.*`.

Changed source files:
- `indra/llappearance/lltexlayer.cpp`
- `indra/llrender/llglcontainment.h`
- `indra/llrender/llglcontainment.cpp`
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

Added containment helpers:
- `setMatrixMode`
- `pushMatrix`
- `popMatrix`
- `setColorUnsignedByteVector`

Reused containment helpers:
- `clearBuffers`
- `setClearColor`
- `setViewport`
- `setLineWidth`
- `getFloat`
- `readPixels`
- `readTextureImage`
- `getError`
- `generateTextureMipmap`
- `copyTextureSubImage3D`
- `setPolygonOffset`
- `setCullFace`
- `setScissorBox`

Contained call families:
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

## Behavior Notes

Each file still owns:
- render order
- target binding order
- readback dimensions, formats, and output buffers
- mipmap and texture copy targets
- cull orientation
- polygon offset values
- matrix stack ordering
- scissor rectangle math

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llappearance/fast -- -j8`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui/fast -- -j8`
- `/opt/homebrew/bin/cmake -E touch /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_arm64.hxx /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_x86_64.hxx`
- targeted `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make ... -j8` for the affected newview objects
- `python3 tools/architecture/source_inventory.py .`

Build note:
- `llui/fast` and `llappearance/fast` rebuilt their static libraries because
  the shared containment header changed.
- No clean build was run.

## Inventory Result

After regeneration, every source file in this packet reports:
- active direct `gl*` calls: 0
- raw `gl*` references: 0

`indra/llrender/llglcontainment.cpp` increased from 137 to 141 active direct
`gl*` calls because it gained four call-through helpers.

Remaining direct `gl*` call areas above zero are intentionally outside this
small-file batch:
- `pipeline.cpp`
- FSR2/TA upscaler files
- platform windowing glue
- broad low-level `llrender`/`llgl` ownership files
- `llcommon/llprofiler.h`

Runtime smoke:
- deferred unless requested; later checks should include scene render, minimap,
  snapshot/export paths, material preview, terrain paint map generation, and UI
  clipping.
