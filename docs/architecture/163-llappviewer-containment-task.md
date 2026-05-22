# LLAppViewer Containment Task

Branch: `phase3`
Source owner: `LLAppViewer`

## Scope

Route direct OpenGL calls in `indra/newview/llappviewer.cpp` through
`llglcontainment.*`.

Included raw OpenGL families:
- `glGetString`
- `glDeleteTextures`

Included behavior:
- viewer statistics GPU vendor string
- viewer statistics GPU renderer string
- viewer statistics OpenGL version string
- deliberate driver crash path

## Non-Scope

Do not change:
- statistics field names or string handling
- driver crash menu behavior
- crash forcing semantics
- graphics manager ownership
- startup or shutdown flow

Do not move viewer information gathering into another owner.

## Ownership Notes

`LLAppViewer` keeps ownership of:
- viewer/system information collection
- crash forcing test hooks
- LLSD field population

`llglcontainment.*` owns only:
- direct OpenGL call-through wrappers

## Risk

Risk: low.

Why:
- This is wrapper-only.
- The `glGetString` calls become a `const char*` call-through so the existing
  `ll_safe_string` handling remains unchanged.
- The driver crash call remains deliberately invalid by preserving the existing
  null texture pointer.

## Verification

Required:
- `git diff --check`
- `llrender/fast`
- targeted `llappviewer.cpp.o` build
- regenerate `docs/architecture/generated/source_inventory.csv`

Runtime smoke:
- deferred unless requested; later checks should verify viewer info still
  reports GPU vendor/renderer/OpenGL version.
