# LLGLHeaders Type-Only Trim Summary

Branch: `phase3`

## Purpose

This is the second header-containment cleanup after runtime OpenGL call
containment. It removes `llglheaders.h` from files where the remaining apparent
GL dependency was either GLTF vocabulary or simple GL typedef usage.

## Result

Direct `llglheaders.h` includes:

- before this pass: 41
- after this pass: 38
- removed in this pass: 3

Runtime OpenGL calls remain contained:

```text
OK: no runtime gl* calls outside indra/llrender/llglcontainment.cpp.
Allowed runtime gl* calls in containment: 160
```

## Changes

Removed false-positive GLTF includes:

- `indra/newview/gltf/llgltfloader.h`
- `indra/newview/llfloaterregioninfo.cpp`

Replaced type-only direct exposure:

- `indra/newview/llviewercamera.cpp`
  - replaced `llglheaders.h` with `llgltypes.h`
  - changed local `GLint` / `GLfloat` casts and stack storage to
    `LLGLint` / `LLGLfloat`

Moved concrete GL exposure from header to implementation:

- `indra/llrender/llpostprocess.h`
  - replaced `llglheaders.h` with `llgltypes.h`
  - changed public typedef/signature spellings from `GLuint` to `LLGLuint`
- `indra/llrender/llpostprocess.cpp`
  - now includes `llglheaders.h` directly because the implementation still
    uses `GL_*` constants, `GLubyte`, `GLenum`, `GLint`, `GLchar`, and
    `gluErrorString`

## Verification

Commands run:

```sh
cmake --build /private/tmp/Mare-viewer-phase2-llrender-make3 --target llrender/fast -- -j8
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/gltf/llgltfloader.cpp.o newview/CMakeFiles/mare-viewer.dir/llfloaterregioninfo.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewercamera.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Known existing warning during verification:

- `indra/newview/gltf/llgltfloader.cpp`: deprecated enum arithmetic warning.

## Remaining Work

The remaining 38 direct `llglheaders.h` includes mostly need one of:

- real `GL_*` constants
- GL typedefs embedded in wider renderer API headers
- low-level GL boundary access
- platform or legacy fixed-function constants

Do not remove them mechanically without either replacing constants/types with a
documented local abstraction or proving the include is redundant by targeted
compile.
