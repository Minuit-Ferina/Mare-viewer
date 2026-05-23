# Visual Param Hint Background Summary

Date: 2026-05-23

Branch: `phase6`

## Scope

This packet follows
`docs/architecture/292-visual-param-hint-background-task.md`.

The source change is limited to the appearance-editor preview owner:

- `indra/newview/lltoolmorph.h`
- `indra/newview/lltoolmorph.cpp`

## Source Changes

Split the hint background projection/modelview draw block out of
`LLVisualParamHint::render()` into private owner-local helper:

- `drawHintBackground()`

`render()` still owns the outer UI matrix lifetime:

- `gGL.pushUIMatrix()`;
- `gGL.loadUIIdentity()`;
- `gGL.popUIMatrix()`.

## Behavior Preserved

This packet does not change:

- `gGL.pushUIMatrix()` / `gGL.popUIMatrix()` lifetime;
- UI identity load timing;
- projection matrix push/pop ordering;
- modelview matrix push/pop ordering;
- ortho projection dimensions;
- `gUIProgram.bind()` timing;
- `LLGLSUIDefault` scope around background draw;
- background draw coordinates and dimensions;
- update/visible flag timing;
- camera setup;
- impostor generation;
- visual-param restore ordering.

## Risk

Risk is medium.

Why:

- render-state calls moved into a private helper;
- matrix push/pop order remains identical;
- the outer UI matrix lifetime remains intentionally unchanged.

## Verification

Targeted object build:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/lltoolmorph.cpp.o -j8
```

Result: passed.

Notes:

- the targeted build rebuilt the `mare-viewer` PCH objects for `arm64` and
  `x86_64` before `lltoolmorph.cpp.o`;
- no clean or full app build was run for this packet.

Guardrails:

```sh
python3 tools/architecture/source_inventory.py .
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/check_gl_header_boundaries.py .
git diff --check
```

Results:

- source inventory regenerated; scanned 3085 source files;
- no runtime `gl*` calls outside `indra/llrender/llglcontainment.cpp`;
- runtime headers do not include `llgl.h` directly outside PCH/prefix;
- raw GL scalar type names are confined to `llglheaders.h`;
- whitespace check passed.

## Next Small Tasks

- Decide whether to split impostor generation into a private helper.
- Do not change blend/depth ordering or visual-param restore timing without a
  dedicated task.
