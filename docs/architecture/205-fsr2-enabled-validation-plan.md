# FSR2 Enabled Validation Plan

Date: 2026-05-22

## Scope

This note defines the validation that still has to happen for
`indra/newview/marefsr2upscaler.cpp`.

The local Darwin build cannot perform this validation because
`MARE_ENABLE_FSR2` is forced to `OFF` on Darwin. In that configuration,
`marefsr2upscaler.cpp` is not added to `viewer_SOURCE_FILES`, and the local
Makefile build tree has no object rule for it.

## Preconditions

Use a non-Darwin build host, or another explicitly supported build
configuration where all of these are true:

- OpenGL compute shader entry points are available.
- OpenGL image load/store entry points are available.
- Direct-state-access texture entry points used by the FSR2 backend are
  available.
- CMake configures with `MARE_ENABLE_FSR2=ON`.
- `indra/newview/marefsr2upscaler.cpp` is present in the viewer source list.
- The viewer target receives `MARE_ENABLE_FSR2=1`.

Do not use the local macOS arm64 Darwin build as proof that the FSR2
implementation compiled. That build only validates FSR2 headers and
non-FSR2 callers.

## Suggested Commands

The exact generator and build directory can vary by platform. Keep the build
tree separate from the local Darwin arm64 dev tree.

Configure with FSR2 enabled:

```sh
cmake -S . -B /path/to/fsr2-enabled-build -DMARE_ENABLE_FSR2=ON
```

Build the FSR2 object or the smallest available target that compiles it:

```sh
cmake --build /path/to/fsr2-enabled-build --target mare-viewer -- -j8
```

If the generator exposes per-object Makefile rules, the narrower check is:

```sh
make -C /path/to/fsr2-enabled-build -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/marefsr2upscaler.cpp.o -j8
```

Also compile known FSR2 header consumers:

```sh
make -C /path/to/fsr2-enabled-build -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/maretaaupscaler.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewercamera.cpp.o -j8
```

Run repository guardrails from the source tree:

```sh
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

## Expected Result

- `marefsr2upscaler.cpp` compiles with `MARE_ENABLE_FSR2=1`.
- No new runtime `gl*` calls appear outside `indra/llrender/llglcontainment.cpp`.
- Runtime headers still avoid direct `llgl.h` includes outside the
  precompiled/prefix headers.
- Raw GL scalar type names remain confined to `llglheaders.h` at the header
  boundary.
- The FSR2 compute shader assets remain unchanged.

## Risk Notes

Risk: medium.

Why:

- The FSR2 implementation is the main non-Darwin modern-OpenGL island.
- It depends on compute, image load/store, shader program, and DSA texture
  calls that the local Darwin build intentionally excludes.
- The containment wrappers preserve call ordering, but this path still needs a
  real FSR2-enabled compiler and loader configuration before further FSR2
  implementation changes are treated as validated.

## Local Status

Current local Darwin status:

- `MARE_ENABLE_FSR2=0`.
- `marefsr2upscaler.cpp` is excluded from the viewer build graph.
- FSR2 header consumers have been compiled through targeted Darwin object
  checks.
- The implementation object remains unvalidated on this machine.

Keep FSR2 disabled on Darwin unless a compatible non-compute fallback is
explicitly designed.
