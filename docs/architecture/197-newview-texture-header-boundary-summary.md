# Newview Texture Header Boundary Summary

Date: 2026-05-22

## Scope

This packet narrows three `indra/newview/` headers that included `llgl.h`
without owning low-level GL state:

- `indra/newview/llsprite.h`
- `indra/newview/lldynamictexture.h`
- `indra/newview/llviewertexturelist.h`

## Source Changes

- `indra/newview/llsprite.h`
  - Replaced the public `llgl.h` include with `llgltypes.h` for the retained
    `LLGLenum` texture-mode API.
- `indra/newview/lldynamictexture.h`
  - Replaced the public `llgl.h` include with `llgltypes.h`.
  - Added an explicit `<set>` include for the instance list.
  - Forward-declared `LLRenderTarget` for the retained pointer member and
    setter.
- `indra/newview/lldynamictexture.cpp`
  - Added the implementation-local `llgl.h` include for GL manager access.
- `indra/newview/llviewertexturelist.h`
  - Removed the public `llgl.h` include. The implementation already includes
    `llgl.h` where GL texture stats are gathered.

## Risk

Low.

The public APIs are unchanged except for removing transitive GL declarations.
The retained `LLGL*` type aliases have the same underlying representation as
the previous GL scalar types. `lllocalcliprect.h` was intentionally left alone
because it owns an `LLGLState` member by value.

## Verification

Commands run:

```sh
make -C /private/tmp/Mare-viewer-phase2-llrender-make3 -f newview/CMakeFiles/mare-viewer.dir/build.make -B newview/CMakeFiles/mare-viewer.dir/llsprite.cpp.o newview/CMakeFiles/mare-viewer.dir/lldynamictexture.cpp.o newview/CMakeFiles/mare-viewer.dir/llviewertexturelist.cpp.o newview/CMakeFiles/mare-viewer.dir/llhudeffectbeam.cpp.o -j8
python3 tools/architecture/check_gl_containment.py .
python3 tools/architecture/source_inventory.py .
git diff --check
```

Results:

- Targeted sprite/dynamic-texture/texture-list `newview` object compiles:
  passed.
- GL containment guardrail: passed.
- Source inventory regenerated.
- Diff whitespace check: passed.

## Follow-Up

Remaining non-boundary headers with direct `llgl.h` includes need separate
handling:

- `indra/llui/lllocalcliprect.h` owns `LLGLState` by value and likely needs a
  small state-helper extraction before it can drop `llgl.h`.
- `indra/newview/pipeline.h` is a broad pipeline contract and should be handled
  separately.
- `indra/newview/marefsr2upscaler.h` is Darwin-excluded in local builds but
  still exposes raw GL types in the source tree.
