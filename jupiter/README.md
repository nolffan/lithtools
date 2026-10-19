# jupiter/: the LithTech Jupiter tools, from Build 69's source

The tools for NOLF2, Contract J.A.C.K. and Tron 2.0: DEdit, ModelEdit,
WinPacker and WorldPacker, Model_Packer, LithRez, FxED, the render-style
tools, the 3ds Max and Maya plug-ins and the rest, built from the source
in Touchdown Entertainment's 2006 "Jupiter System Enterprise Edition,
Build 69" (archive.org `Lithtech-Jupiter-System-Enterprise-Edition-Build-69`;
`../INVENTORY.md`).

The first goal is a working build of that source, for testing under Wine;
a byte-matching build of Contract J.A.C.K.'s October 2003 tools comes
later (`../INVENTORY.md`, `../FORMATS.md`).

## src/

`src/` is the setup's on-disk layout, `Engine/` and `Libs/` side by side, so
every project's relative paths resolve as Monolith wrote them. It was
copied out of the package by `../scripts/import_b69.py`, which records what it
takes and leaves out, and committed unmodified apart from line endings (LF
for text files). Every later change is a commit of its own.

Not in the repository:

- **The plug-in SDKs** (`Engine/tools/Plugins/SDKFiles`: the 3ds Max 7, Maya
  6.5 and 7.0, and Photoshop SDKs that Build 69 carries). They are
  Autodesk's and Adobe's, and Monolith's own readme says they are not
  redistributable. The build copies them in from the Build 69 package.
  The Max 3-6 and Maya 3.0-6.0 SDKs that the other plug-in projects name are
  not in the package at all.
- **The DirectX SDK** the tools build against (February 2006), likewise
  taken from the package (`DirectX/dxsdk_feb2006.exe`).
- Build outputs that the package happened to carry (`Libs/built`, the Maya
  plug-ins' `Release_Maya70` folders, `.obj`/`.exp` files) and the
  Microsoft runtime DLLs in `tools/MiscPack`.

## Terms

`src/` is Monolith Productions' / LithTech, Inc.'s code (later Touchdown
Entertainment); `src/Engine/sdk/Legal.txt` is its notice. archive.org's
description says Touchdown released it under the GPL, but nothing in the
package says so, and no licence file is in it. Treat it as nolf2src's
`docs/LICENSE-NOTES.md` treats the same tree: work out the terms before
distributing anything built from it. Third-party code inside it keeps its
own terms (zlib, FreeType, ImageMagick, STLport, S3TC's `s3tc.lib`, ...).
