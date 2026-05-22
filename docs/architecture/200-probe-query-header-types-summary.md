# Probe And Query Header Types Summary

Date: 2026-05-22

## Scope

This packet narrows remaining newview probe/query headers that exposed raw GL
scalar type names for stored integer data.

Files touched:

- `indra/newview/llheroprobemanager.h`
- `indra/newview/llreflectionmapmanager.h`
- `indra/newview/llreflectionmap.h`
- `indra/newview/llscenemonitor.h`

## Source Changes

- Added explicit `llgltypes.h` includes where the headers now use `LLGL*`
  aliases directly.
- Replaced probe UBO integer fields from `GLint` to `LLGLint`.
- Replaced occlusion/query object names from `GLuint` to `LLGLuint`.

## Risk

Low.

The aliases keep the same underlying scalar representations. The affected
fields are stored integer/query values; no query generation, deletion, UBO
upload, or render ordering behavior changed.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llreflectionmapmanager.cpp.o newview/CMakeFiles/mare-viewer.dir/llheroprobemanager.cpp.o newview/CMakeFiles/mare-viewer.dir/llreflectionmap.cpp.o newview/CMakeFiles/mare-viewer.dir/llscenemonitor.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- Targeted reflection/scene-monitor `newview` object compiles: passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

Remaining raw GL type spellings in headers are now concentrated in:

- `indra/llrender/llgl.h`
- `indra/llrender/llglheaders.h`
- commented legacy references such as `indra/newview/llbox.h`
