# Small Inventory Remainder Containment Summary

Branch: `phase3`
Source commit: `fe1a55d96d`

## Scope Completed

Closed the small non-platform inventory remainder outside the main small-file
batch.

Changed source files:
- `indra/newview/maretaaupscaler.cpp`
- `indra/llrender/llimagegl.h`
- `indra/llrender/llimagegl.cpp`
- `indra/llrender/llvertexbuffer.cpp`
- `indra/llrender/llglcommonfunc.cpp`
- `indra/llrender/llcubemaparray.cpp`

Active wrapper routing:
- `maretaaupscaler.cpp` now routes accumulation-buffer clear color and clear
  calls through `llglcontainment.*`.

Comment-only inventory cleanup:
- removed call-looking `gl*` text from comments in small `llrender` files that
  the generated inventory counted as direct calls.

## Behavior Notes

Behavior is unchanged:
- TAA still clears both accumulation buffers to transparent black.
- Texture, vertex buffer, stencil, and cubemap comments were reworded only.

`llglcontainment.*` owns only the direct OpenGL call-throughs.

## Verification

Completed:
- `git diff --check`
- `/opt/homebrew/bin/cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8`
- `/opt/homebrew/bin/cmake -E touch /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_arm64.hxx /private/tmp/Mare-viewer-phase2-llrender-make3/newview/CMakeFiles/mare-viewer.dir/cmake_pch_x86_64.hxx`
- `make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make newview/CMakeFiles/mare-viewer.dir/maretaaupscaler.cpp.o -j8`
- `python3 tools/architecture/source_inventory.py .`

Build note:
- The first targeted TAA object attempt failed because `llimagegl.h` changed
  and the newview PCH needed a refresh.
- After the minimal PCH refresh, the targeted object build passed.
- No clean build was run.

## Inventory Result

After regeneration:
- `indra/newview/maretaaupscaler.cpp`: active direct `gl*` calls reduced from
  2 to 0.
- `indra/newview/maretaaupscaler.cpp`: raw `gl*` references reduced from 2 to
  0.
- `indra/llrender/llimagegl.h`: active direct `gl*` calls reduced from 3 to 0.
- `indra/llrender/llglcommonfunc.cpp`: active direct `gl*` calls reduced from
  2 to 0.
- `indra/llrender/llcubemaparray.cpp`: active direct `gl*` calls reduced from
  1 to 0.
- `indra/llrender/llvertexbuffer.cpp`: active direct `gl*` calls reduced from
  2 to 0.
- `indra/llrender/llimagegl.cpp`: active direct `gl*` calls reduced from 3 to
  0.

Remaining active direct `gl*` call areas above zero:
- `pipeline.cpp`
- `marefsr2upscaler.cpp`
- `llgl.cpp`
- `llglheaders.h`
- platform windowing files
- `llrender.cpp` debug-output initialization
- `llcommon/llprofiler.h`

Runtime smoke:
- deferred unless requested.
