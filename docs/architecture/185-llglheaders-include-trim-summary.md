# LLGLHeaders Include Trim Summary

Branch: `phase3`

## Purpose

This is the first header-containment cleanup after runtime OpenGL call
containment. It does not hide `llglheaders.h` behind `llglcontainment.h`.
Instead, it removes direct `llglheaders.h` includes from files that no longer
use GL types, GL constants, or raw GL symbols.

## Result

Direct `llglheaders.h` includes:

- before this pass: 61
- after this pass: 41
- removed in this pass: 20

Direct system OpenGL/CGL/GLX/OSMesa includes outside `llglheaders.h` remain:

- count: 8
- mostly platform/context files under `indra/llwindow/`
- shader files that still include `OpenGL/OpenGL.h`

Runtime OpenGL calls remain contained:

```text
OK: no runtime gl* calls outside indra/llrender/llglcontainment.cpp.
Allowed runtime gl* calls in containment: 160
```

## Includes Removed

Removed `#include "llglheaders.h"` from:

- `indra/llappearance/llavatarjoint.cpp`
- `indra/llrender/llglcommonfunc.cpp`
- `indra/llrender/llrendersphere.cpp`
- `indra/llui/llprogressbar.cpp`
- `indra/llui/llstatgraph.cpp`
- `indra/newview/llbox.cpp`
- `indra/newview/llcylinder.cpp`
- `indra/newview/llflexibleobject.cpp`
- `indra/newview/llfloaterabout.cpp`
- `indra/newview/llfloatercolorpicker.cpp`
- `indra/newview/llfloatermap.cpp`
- `indra/newview/llhudeffectbeam.cpp`
- `indra/newview/llhudrender.cpp`
- `indra/newview/lljoystickbutton.cpp`
- `indra/newview/lllegacyatmospherics.cpp`
- `indra/newview/llpanellogin.cpp`
- `indra/newview/llprogressview.cpp`
- `indra/newview/lltexturectrl.cpp`
- `indra/newview/lltoastgroupnotifypanel.cpp`
- `indra/newview/lltoolselectrect.cpp`

## Kept On Purpose

Examples of kept `llglheaders.h` includes:

- `pipeline.cpp`: still uses many `GL_*` constants and GL typedefs.
- `llviewerdisplay.cpp`: still uses `GL_*` constants and GL state helpers.
- `llviewerwindow.cpp`: still uses pixel formats, readback formats, and GL
  constants.
- `llviewercamera.cpp`: still uses `GLint` and `GLfloat`.
- `llpostprocess.h`: still exposes `GLuint` in its public typedefs and method
  signatures.
- `marefsr2upscaler.cpp`: non-Darwin compute path still uses many GL constants.
- `media_plugin_cef.cpp`: explicitly includes `llglheaders.h` for `GL_*`
  constants.
- `llgl.cpp`, `llgl.h`, `llglcontainment.cpp`: low-level GL boundary files.

## Verification

Commands run:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llappearance -- -j8
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llui -- -j8
```

For the touched `newview` files, individual object targets were compiled
through `newview/CMakeFiles/mare-viewer.dir/build.make` instead of launching a
full `mare-viewer` rebuild.

## Next Small Pass

The remaining header work should be split by reason:

- files that need only `GL_*` constants
- files that need GL typedefs such as `GLuint`, `GLint`, or `GLfloat`
- platform/context files that must keep direct system OpenGL/CGL/GLX includes
- files where GL vocabulary is actually GLTF or another false-positive family
