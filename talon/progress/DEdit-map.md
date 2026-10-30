# DEdit.exe (AvP2 Tools Update 1/2, "Talon"): the map of the image

The Talon world editor, linked 2001-12-06 (Update 1's and Update 2's are
the same file), "HKEY_CURRENT_USER\Software\LithTech Inc.\Talon\DEdit".
An MFC MDI application: the project bar (worlds, textures, models,
sprites, sounds, properties, the node view), region documents with their
views and trackers, the world format (`.ed`, and LTA through the LTA
library), a DirectDraw/Direct3D renderer, MRCEXT's docking windows.
4,471 rows of the function table are scored (`make progress`): its own
code 0x00401000..0x0047d040 and the libraries 0x00483020..0x0048d740 and
0x004a25f0..0x004a7d00, less the rows of `DEdit-screen-exclude.csv`; S3's
s3tc.lib between them is middleware.

## What built it

The Rich header: 230 C++ and 10 C objects of cl 12.00.9044 (0x31 and
0x30, VC6 SP5 with the Processor Pack: `toolchains/msvc-6.00-9044`), 87
C++ objects of 8569 (MFC 4.2's, nafxcw.lib), the C runtime's and the
C++ library's (178 C and 11 C++ of 8047, 44 masm 7299, 3 AliasObj; 21
C++ and 1 C of 8168), 32 objects without an `@comp.id` (S3's s3tc.lib
has two; ModelEdit's has 62, the toolchain's own), one C++ object of
cl 13.00.8830 and two of id 0x25 build 8755 (probably the DirectX 8.0
SDK's dxguid.lib and import libraries, built with a VC7 beta: the 8.1
SDK's dxguid.lib that talon has is 13.00.9178),
cvtres 1735, link 6.00. No row of 8966: the 240 objects of 9044 are
Monolith's, the libraries' included (the ten C ones are zlib's).

- **cl 9044, `/O1`**: functions are unaligned (0x401000, 0x401006, ...),
  C++ EH frames through `__EH_prolog` (0x48e368); `/GX`. The libraries
  (GenRegMgr, ButeMgr, RegMgr, StdLith, DibMgr, the LTA library, zlib) are
  `/O2`: their functions are 16-byte aligned.
- **Literals pooled, in `.data`**: `/Gf`. "%s\\shell32.dll" (used by the
  model, sound, sprite, texture and world dialogs), "rb", "wb",
  "Courier New", "Modified" are one copy each.
- **No `/GR`**: 112 type descriptors (101 `.?AV` classes, 2 `.?AU`, 9
  `.PA` for catch clauses), every one MFC's, the C runtime's, the old
  iostreams', STLport's or StdLith's (CLithException, CLithIOException,
  CLithMemException: thrown). CNewTypeDlg and CMirrorFile are MFC's.
- **STLport**, not VC6's C++ library: `_STL::length_error`,
  `_STL::logic_error`, `_STL::__Named_exception`, "basic_string"
  (0x439c12, STLport's length error); libscan finds no libcpmt.lib
  function. Its iostreams are the old ones (libcimt.lib's ifstream,
  filebuf, 0x0049da35..0x0049ee94): STLport without its own iostreams,
  as the LT2 copy (`src/LT2/lithshared/stl`) is set up. So neither of
  ModelEdit's two markers is here: its `std::string` is STLport's (no
  `_Tidy`, no reference count), and there is no `<xlocale>` initializer
  at module starts (26 functions call atexit, 0x48e576, in all; most
  modules have no static initializer).
- **Static MFC and C runtime**: nafxcw.lib (from 0x0048d759,
  wincore.obj's `_InitMultipleMonitorStubs`; MFC's inline
  CPropertySheet::GetTabControl just before, 0x0048d740), libcmt.lib
  (0x0048df91..), libcimt.lib, then zlib, more libcmt and nafxcw
  (0x004a7da0, strex.obj ..), found with `tools/libscan.py` and
  msvc-6.00-9044's libraries.
- **Imports**: DDRAW (DirectDrawCreate, DirectDrawEnumerateA: the
  renderer is DirectDraw 4 and Direct3D 3, "QueryInterface(IID_IDirectDraw4)
  failed."), WINMM (timeGetTime), SHELL32 (drag and drop, ShellExecuteA,
  ExtractIconA, SHGetFileInfoA: no SHBrowseForFolder, Build 69's
  DirDialog), COMCTL32 (image lists, property sheets), comdlg32,
  WINSPOOL, GDI32, USER32, ADVAPI32, KERNEL32. No D3D9, no OpenGL.

Our link's Rich header has 276 C++ objects of 9044 (the Build 69 only
group's 45 and CryptMgr's two are the difference) and dxguid.lib's 9178.

`src/LT2/tools/DEdit/DEdit.dsw` builds it so: build.mk's ENV_/CL_/LINK_DEdit
and the libraries' projects are msvc-6.00-9044's, with its headers, MFC
and runtime (and Developer Studio's ATL headers, for `<afxpriv.h>`), the
DirectX 8.1 SDK's headers and ddraw.lib and dxguid.lib.

## Layout (libscan.py)

| Range | What | Source |
|---|---|---|
| 0x00401000..0x0047d040 | DEdit's own objects, in module order (below), with the template instances (CMoArray, STLport) they emit | to reconstruct |
| 0x0047d040..0x0048301a | S3's s3tc.lib (s3tc.obj, S3_quant.obj; 14 functions found) | prebuilt: `tools/shared/world/s3tc.lib` (middleware) |
| 0x00483020..0x004835a0 | GenRegMgr | lithshared/genregmgr (genregmgr60_Talon.dsp) |
| 0x004835a0..0x00488ed0 | ButeMgr | src/Libs/ButeMgr (ButeMgr_Talon.dsp) |
| 0x00488ed0..0x00489190 | RegMgr | lithshared/regmgr (regmgr60_Talon.dsp, ModelEdit's) |
| 0x00489190..0x0048b5e0 | StdLith, allocator-override build | lithshared/stdlith (Lib_StdLith_alloc.dsp, ModelEdit's) |
| 0x0048b5e0..0x0048ba82 | DibMgr | lithshared/dibmgr (dibmgr60_Talon.dsp) |
| 0x0048ba90..0x0048d740 | the LTA library (ltaconverter.obj .. lzsswindow.obj) | src/Engine/libs/ltamgr (ltamgr_Talon.dsp) |
| 0x0048d740..0x0048d759 | MFC's inline CPropertySheet::GetTabControl | the toolchain's |
| 0x0048d759..0x0048df91 | nafxcw.lib (wincore.obj first) | the toolchain's |
| 0x0048df91..0x0049d9c2 | libcmt.lib | the toolchain's |
| 0x0049da35..0x0049ee94 | libcimt.lib (old iostreams) | the toolchain's |
| 0x004a25f0..0x004a7d00 | zlib 1.0.4 (deflate.obj first; 53 functions found) | lithshared/zlib (Lib_ZLib_Talon.dsp) |
| 0x004a7d6b.. | more libcmt, then nafxcw (strex.obj ..) | the toolchain's |

The libraries come in the order of the project's dependencies (the 2001
workspace's: GenRegMgr, ButeMgr, RegMgr, StdLith, DibMgr, LTAMgr), after
the objects and s3tc.lib (a library the project lists as a source). zlib
comes after the C runtime: it was named by a `#pragma comment(lib)` (a
default library, searched after nafxcw and libcmt), not linked as a
project; ours is a dependency (to change).

The libraries' sources: GenRegMgr's and DibMgr's are build 51's (src/Libs)
brought to the LT2 headers (`lithshared/incs`: DWORD, not build 51's
unsigned int) in `lithshared/genregmgr` and `lithshared/dibmgr`; ButeMgr is
build 51's with MFC (its strings are CStrings); the LTA library is the
plug-ins' (`src/Engine/libs/ltamgr`), /MT. Compiled with 9044, libscan
finds 8 of GenRegMgr's functions (none before), 86 of ButeMgr's (35 with
8966), 6 of RegMgr's, 61 of StdLith's, 10 of DibMgr's, 36 of the LTA
library's, 53 of zlib's (24).

Inside the range, 13 rows are MFC's header functions (CWnd::BeginModalState
and EndModalState, 0x401009 and 0x401015 as in ModelEdit; GetFont,
GetOwner, CFileDialog's, CBitmap's and CWaitCursor's destructors,
CBitmap::LoadBitmap, CString(), CObject's operator delete, a `??_9` vcall
thunk) that our objects instantiate and that are byte-identical in
msvc-6.00-9044's nafxcw.lib: screened out as not Monolith's
(`DEdit-screen-exclude.csv`), with 21 function-table rows that start
inside a function (mid-instruction in ProjectBar's 0x403ea7 and DEdit.cpp's
0x41653e, 0x4166da, 0x41731a; the branch targets 0x46d2c1 and 0x47c059).
More such rows are likely: the table's 0x403ea7 row (412 bytes) runs on
past 0x404079, and the near list pairs some of these interiors.

## Modules

The link keeps the project's order, and it is the 2001 project's: **the
"Project Bar" group first** (BaseImgDlg, ModelDlg, NodeView, ProjectBar,
PropertiesDlg, SoundDlg, SpriteDlg, TextureDlg, WorldsDlg: Build 69's
vcproj has the same group), **then every other source of the project in
file-name order** (case-insensitive, whatever its directory: Controls,
Dlgs, Docs, Frames, Misc, Options, Project, RegionEdit, Views, the shared
engine, world and model code alike: d_filemgr sorts before DEdit,
concommand and conparse between ColorWell and CylinderPrimDlg, the
Options files between ObjectImporter and PaletteEdit), **then MRCEXT**
(DRAGDOCK .. SZTOOLBA) **and the trackers** (RVCommand .. TrackerMgr), as
Build 69's vcproj's mrcext and Trackers groups. The ranges are where
`tools/libscan.py`-style matching puts each of our objects' functions that
match at one address and no other object's (a module starts at its first such function, so a start can be late by the
functions before it that match nowhere, and a module with none (most
Build 69-only ones, and some small 2001 ones) is inside its neighbour's
range); counts are `make
progress`'s, with Build 69's code as it stands.

| Range | Module | Source (under src/LT2) | Rows | Ident. | Near | Small | Todo |
|---|---|---|--:|--:|--:|--:|--:|
| 0x00401000..0x00401f04 | BaseImgDlg, ModelDlg | tools/DEdit/BaseImgDlg.cpp, ModelDlg.cpp | 45 | 16 | 20 | 9 | 0 |
| 0x00401f04..0x00403106 | NodeView | tools/DEdit/NodeView.cpp | 44 | 10 | 31 | 3 | 0 |
| 0x00403106..0x00404b2c | ProjectBar | tools/DEdit/Controls/ProjectBar.cpp | 77 | 30 | 42 | 5 | 0 |
| 0x00404b2c..0x00408596 | PropertiesDlg | tools/DEdit/Dlgs/PropertiesDlg.cpp | 80 | 30 | 41 | 6 | 3 |
| 0x00408596..0x0040d69f | TextureDlg | tools/DEdit/TextureDlg.cpp | 99 | 54 | 35 | 9 | 1 |
| 0x0040d69f..0x0040e676 | AdvancedSelect | tools/DEdit/RegionEdit/AdvancedSelect.cpp | 45 | 29 | 12 | 3 | 1 |
| 0x0040e676..0x0040f48d | AllTextureDlg | tools/DEdit/AllTextureDlg.cpp | 28 | 9 | 17 | 1 | 1 |
| 0x0040f48d..0x0040f650 | AlphaFromColorDlg | tools/DEdit/Dlgs/AlphaFromColorDlg.cpp | 7 | 5 | 0 | 1 | 1 |
| 0x0040f650..0x0040f751 | AutoTextureOptionsDlg | tools/DEdit/Dlgs/AutoTextureOptionsDlg.cpp | 4 | 2 | 1 | 1 | 0 |
| 0x0040f751..0x00411576 | BasePoly | tools/DEdit/World/BasePoly.cpp | 61 | 26 | 31 | 3 | 1 |
| 0x00411576..0x004116d9 | baserezdlg | tools/DEdit/baserezdlg.cpp | 4 | 1 | 3 | 0 | 0 |
| 0x004116d9..0x00411735 | bindmgr | tools/shared/engine/bindmgr.cpp | 3 | 3 | 0 | 0 | 0 |
| 0x00411735..0x00411840 | classbind | tools/shared/engine/classbind.cpp | 6 | 6 | 0 | 0 | 0 |
| 0x00411840..0x00411fbc | ClassDlg | tools/DEdit/Dlgs/ClassDlg.cpp | 25 | 20 | 2 | 3 | 0 |
| 0x00411fbc..0x004124b4 | ClassList | tools/DEdit/ClassList.cpp | 19 | 7 | 8 | 4 | 0 |
| 0x004124b4..0x00412772 | ClassTree | tools/DEdit/Controls/ClassTree.cpp | 6 | 6 | 0 | 0 | 0 |
| 0x00412772..0x00413037 | CObjInterface | tools/DEdit/FileFormats/CObjInterface.cpp | 17 | 12 | 3 | 2 | 0 |
| 0x00413037..0x0041316c | ColorPicker | tools/DEdit/ColorPicker.cpp | 6 | 5 | 1 | 0 | 0 |
| 0x0041316c..0x00413e72 | ColorSelectDlg | tools/DEdit/Dlgs/ColorSelectDlg.cpp | 27 | 14 | 9 | 4 | 0 |
| 0x00413e72..0x00413f73 | ColorWell | tools/DEdit/ColorWell.cpp | 2 | 2 | 0 | 0 | 0 |
| 0x00413f73..0x00414b73 | ColumnCtrl | tools/DEdit/controls/ColumnCtrl.cpp | 25 | 21 | 1 | 1 | 2 |
| 0x00414b73..0x00414f16 | conparse | tools/shared/engine/conparse.cpp | 7 | 6 | 1 | 0 | 0 |
| 0x00414f16..0x00415075 | CylinderPrimDlg | tools/DEdit/CylinderPrimDlg.cpp | 9 | 5 | 1 | 3 | 0 |
| 0x00415075..0x00415db6 | d3d_texturemgr | tools/DEdit/draw/d3d_texturemgr.cpp | 27 | 5 | 18 | 1 | 3 |
| 0x00415db6..0x00416989 | d_filemgr | tools/DEdit/Project/d_filemgr.cpp | 39 | 21 | 12 | 6 | 0 |
| 0x00416989..0x00417cca | DEdit | tools/DEdit/DEdit.cpp | 41 | 16 | 16 | 4 | 5 |
| 0x00417cca..0x0041876f | dedit_concommand | tools/DEdit/dedit_concommand.cpp | 29 | 14 | 9 | 6 | 0 |
| 0x0041876f..0x00418ce1 | dhashtable | tools/shared/engine/dhashtable.cpp | 31 | 28 | 0 | 3 | 0 |
| 0x00418ce1..0x0041af3d | draw_d3d | tools/DEdit/draw/draw_d3d.cpp | 43 | 13 | 26 | 2 | 2 |
| 0x0041af3d..0x0041f523 | DrawBase | tools/DEdit/draw/DrawBase.cpp | 59 | 15 | 44 | 0 | 0 |
| 0x0041f523..0x0041fdf1 | dtxmgr | tools/DEdit/dtxmgr.cpp | 17 | 16 | 1 | 0 | 0 |
| 0x0041fdf1..0x00420109 | edit_actions | tools/DEdit/RegionEdit/edit_actions.cpp | 10 | 8 | 2 | 0 | 0 |
| 0x00420109..0x00422a83 | EditBrush | tools/DEdit/World/EditBrush.cpp | 61 | 21 | 32 | 4 | 4 |
| 0x00422a83..0x00423493 | EditClipboard | tools/DEdit/RegionEdit/EditClipboard.cpp | 19 | 11 | 4 | 4 | 0 |
| 0x00423493..0x004236f3 | EditHelpers | tools/DEdit/Misc/EditHelpers.cpp | 29 | 20 | 5 | 4 | 0 |
| 0x004236f3..0x00424e55 | EditObjects | tools/DEdit/World/EditObjects.cpp | 45 | 19 | 22 | 4 | 0 |
| 0x00424e55..0x0042597c | EditPoly | tools/DEdit/World/EditPoly.cpp | 44 | 9 | 9 | 25 | 1 |
| 0x0042597c..0x00427871 | EditProjectMgr | tools/DEdit/Project/EditProjectMgr.cpp | 49 | 26 | 14 | 8 | 1 |
| 0x00427871..0x0042b479 | EditRegion | tools/DEdit/World/EditRegion.cpp | 105 | 48 | 51 | 3 | 3 |
| 0x0042b479..0x0042b5ee | ExportObjFile | tools/DEdit/ExportObjFile.cpp | 7 | 6 | 0 | 1 | 0 |
| 0x0042b5ee..0x0042b93f | FilePalette | tools/DEdit/FilePalette.cpp | 6 | 4 | 2 | 0 | 0 |
| 0x0042b93f..0x0042be77 | FrameList | tools/DEdit/Controls/FrameList.cpp | 17 | 9 | 5 | 3 | 0 |
| 0x0042be77..0x0042bf35 | GenerateUniqueNamesDlg | tools/DEdit/dlgs/GenerateUniqueNamesDlg.cpp | 5 | 4 | 0 | 1 | 0 |
| 0x0042bf35..0x0042c0df | genericprop_setup | tools/shared/engine/genericprop_setup.cpp | 6 | 4 | 2 | 0 | 0 |
| 0x0042c0df..0x0042c5cf | genltstream | tools/shared/engine/genltstream.cpp | 7 | 4 | 3 | 0 | 0 |
| 0x0042c5cf..0x0042cccf | geomroutines | tools/shared/engine/geomroutines.cpp | 11 | 6 | 5 | 0 | 0 |
| 0x0042cccf..0x0042cd08 | ImportObjectDlg | tools/DEdit/dlgs/ImportObjectDlg.cpp | 3 | 2 | 0 | 1 | 0 |
| 0x0042cd08..0x0042d25c | ImportObjFileFormat | tools/DEdit/ImportObjFileFormat.cpp | 14 | 6 | 3 | 3 | 2 |
| 0x0042d25c..0x0042d642 | lightmap_compress | tools/shared/engine/lightmap_compress.cpp | 7 | 4 | 1 | 1 | 1 |
| 0x0042d642..0x0042e0a9 | load_pcx | tools/shared/engine/load_pcx.cpp | 10 | 3 | 5 | 0 | 2 |
| 0x0042e0a9..0x00431f1c | MainFrm | tools/DEdit/Frames/MainFrm.cpp | 112 | 38 | 66 | 8 | 0 |
| 0x00431f1c..0x004375e7 | model | tools/shared/model/model.cpp | 267 | 226 | 7 | 30 | 4 |
| 0x004375e7..0x0043a218 | model_load | tools/shared/model/model_load.cpp | 93 | 83 | 2 | 5 | 3 |
| 0x0043a218..0x0043a34f | modelallocations | tools/shared/model/modelallocations.cpp | 4 | 3 | 1 | 0 | 0 |
| 0x0043a34f..0x0043b123 | mtreectl | tools/DEdit/mtreectl.cpp | 34 | 29 | 2 | 2 | 1 |
| 0x0043b123..0x0043b64a | NameChangeReportDlg | tools/DEdit/dlgs/NameChangeReportDlg.cpp | 13 | 10 | 2 | 1 | 0 |
| 0x0043b64a..0x0043bf37 | Navigator | tools/DEdit/draw/Navigator.cpp | 16 | 14 | 1 | 1 | 0 |
| 0x0043bf37..0x0043c43c | navigatordlg | tools/DEdit/dlgs/navigatordlg.cpp | 16 | 15 | 0 | 1 | 0 |
| 0x0043c43c..0x0043c4c1 | NavigatorStoreDlg | tools/DEdit/dlgs/NavigatorStoreDlg.cpp | 4 | 3 | 0 | 1 | 0 |
| 0x0043c4c1..0x0043c61b | NewProjectDlg | tools/DEdit/Dlgs/NewProjectDlg.cpp | 10 | 7 | 1 | 2 | 0 |
| 0x0043c61b..0x0043c8de | node_ops | tools/DEdit/World/node_ops.cpp | 6 | 3 | 3 | 0 | 0 |
| 0x0043c8de..0x0043d46a | NodeViewTreeCtrl | tools/DEdit/NodeViewTreeCtrl.cpp | 33 | 18 | 13 | 2 | 0 |
| 0x0043d46a..0x0043dae8 | ObjectBrowserDlg | tools/DEdit/dlgs/ObjectBrowserDlg.cpp | 16 | 5 | 10 | 1 | 0 |
| 0x0043dae8..0x0043e800 | ObjectImporter | tools/DEdit/ObjectImporter.cpp | 24 | 15 | 9 | 0 | 0 |
| 0x0043e800..0x0043ea8a | OptionsBase | tools/DEdit/Options/OptionsBase.cpp | 13 | 12 | 0 | 1 | 0 |
| 0x0043ea8a..0x0043ecce | OptionsClassDialog | tools/DEdit/Options/OptionsClassDialog.cpp | 6 | 6 | 0 | 0 | 0 |
| 0x0043ecce..0x0043ef95 | OptionsClipboard | tools/DEdit/Options/OptionsClipboard.cpp | 8 | 4 | 3 | 1 | 0 |
| 0x0043ef95..0x0043fcac | OptionsDisplay | tools/DEdit/Options/OptionsDisplay.cpp | 11 | 3 | 8 | 0 | 0 |
| 0x0043fcac..0x0043fd64 | OptionsGenerateUniqueNames | tools/DEdit/Options/OptionsGenerateUniqueNames.cpp | 5 | 4 | 0 | 1 | 0 |
| 0x0043fd64..0x00440217 | OptionsObjectBrowser | tools/DEdit/Options/OptionsObjectBrowser.cpp | 21 | 9 | 7 | 3 | 2 |
| 0x00440217..0x0044062b | OptionsWindows | tools/DEdit/Options/OptionsWindows.cpp | 12 | 9 | 0 | 3 | 0 |
| 0x0044062b..0x004407a6 | PaletteEdit | tools/DEdit/PaletteEdit.cpp | 7 | 5 | 0 | 2 | 0 |
| 0x004407a6..0x00443118 | pixelformat | tools/shared/engine/pixelformat.cpp | 70 | 69 | 0 | 1 | 0 |
| 0x00443118..0x0044c97b | PopupNoteDlg | tools/DEdit/dlgs/PopupNoteDlg.cpp | 314 | 146 | 112 | 30 | 26 |
| 0x0044c97b..0x0044cb82 | ProjectTabControlBar | tools/DEdit/Controls/ProjectTabControlBar.cpp | 4 | 2 | 1 | 1 | 0 |
| 0x0044cb82..0x0044cdb1 | ProjectTabCtrl | tools/DEdit/controls/ProjectTabCtrl.cpp | 9 | 6 | 1 | 2 | 0 |
| 0x0044cdb1..0x0044cfe7 | PropEditString | tools/DEdit/controls/PropEditString.cpp | 8 | 7 | 0 | 1 | 0 |
| 0x0044cfe7..0x0044d215 | PropEditStringList | tools/DEdit/Controls/PropEditStringList.cpp | 8 | 1 | 2 | 5 | 0 |
| 0x0044d215..0x0044d3d7 | PropertyHelpers | tools/DEdit/Docs/PropertyHelpers.cpp | 3 | 1 | 2 | 0 | 0 |
| 0x0044d3d7..0x0044e3ce | PropList | tools/DEdit/World/PropList.cpp | 76 | 55 | 16 | 4 | 1 |
| 0x0044e3ce..0x0044e555 | PropPageDisplay | tools/DEdit/dlgs/PropPageDisplay.cpp | 7 | 3 | 2 | 2 | 0 |
| 0x0044e555..0x0044eaf9 | PropPageOptionsClipboard | tools/DEdit/dlgs/PropPageOptionsClipboard.cpp | 24 | 7 | 12 | 5 | 0 |
| 0x0044eaf9..0x0044fbee | PropPageOptionsRun | tools/DEdit/dlgs/PropPageOptionsRun.cpp | 41 | 15 | 22 | 4 | 0 |
| 0x0044fbee..0x0045268d | RegionDoc | tools/DEdit/Docs/RegionDoc.cpp | 103 | 44 | 50 | 9 | 0 |
| 0x0045268d..0x00457ad1 | RegionFrame | tools/DEdit/Frames/RegionFrame.cpp | 146 | 51 | 81 | 10 | 4 |
| 0x00457ad1..0x0045ec14 | RegionView | tools/DEdit/Views/RegionView.cpp | 195 | 50 | 134 | 8 | 3 |
| 0x0045ec14..0x0045f02d | ResizeDlg | tools/DEdit/ResizeDlg.cpp | 21 | 19 | 0 | 2 | 0 |
| 0x0045f02d..0x0045f397 | ResNewDir | tools/DEdit/ResNewDir.cpp | 6 | 3 | 1 | 1 | 1 |
| 0x0045f397..0x0045f697 | ResourceMgr | tools/DEdit/ResourceMgr.cpp | 10 | 2 | 2 | 1 | 5 |
| 0x0045f697..0x00460495 | RotateSelection | tools/DEdit/RotateSelection.cpp | 31 | 6 | 20 | 3 | 2 |
| 0x00460495..0x00460507 | SpherePrimitiveDlg | tools/DEdit/dlgs/SpherePrimitiveDlg.cpp | 2 | 1 | 0 | 1 | 0 |
| 0x00460507..0x0046083d | Splash | tools/DEdit/Misc/Splash.cpp | 14 | 13 | 0 | 1 | 0 |
| 0x0046083d..0x004612b6 | SpriteEditDlg | tools/DEdit/Dlgs/SpriteEditDlg.cpp | 22 | 21 | 0 | 1 | 0 |
| 0x004612b6..0x00461473 | streamsim | tools/shared/engine/streamsim.cpp | 9 | 9 | 0 | 0 | 0 |
| 0x00461473..0x004617dd | StringDlg | tools/DEdit/Dlgs/StringDlg.cpp | 11 | 9 | 1 | 1 | 0 |
| 0x004617dd..0x00462982 | Texture | tools/DEdit/Misc/Texture.cpp | 23 | 13 | 9 | 1 | 0 |
| 0x00462982..0x004637ce | TextureProp | tools/DEdit/TextureProp.cpp | 32 | 15 | 16 | 1 | 0 |
| 0x004637ce..0x00464148 | transformmaker | tools/shared/model/transformmaker.cpp | 9 | 3 | 4 | 0 | 2 |
| 0x00464148..0x004642dd | UIEvent | tools/DEdit/UIEvent.cpp | 5 | 4 | 1 | 0 | 0 |
| 0x004642dd..0x0046485f | undo_mgr | tools/DEdit/RegionEdit/undo_mgr.cpp | 25 | 17 | 7 | 1 | 0 |
| 0x0046485f..0x00465426 | UsefulDib | tools/DEdit/draw/UsefulDib.cpp | 16 | 9 | 4 | 3 | 0 |
| 0x00465426..0x004654e2 | VectorEdit | tools/DEdit/VectorEdit.cpp | 2 | 1 | 1 | 0 | 0 |
| 0x004654e2..0x0046552f | version_info | tools/shared/engine/version_info.cpp | 1 | 1 | 0 | 0 | 0 |
| 0x0046552f..0x0046620a | version_resource | tools/shared/engine/version_resource.cpp | 16 | 4 | 12 | 0 | 0 |
| 0x0046620a..0x0046eb58 | ViewFunctions | tools/DEdit/RegionEdit/ViewFunctions.cpp | 140 | 18 | 112 | 3 | 7 |
| 0x0046eb58..0x0046ed90 | WorldInfoDlg | tools/DEdit/Dlgs/WorldInfoDlg.cpp | 13 | 10 | 1 | 2 | 0 |
| 0x0046ed90..0x0046f3c0 | WorldNode | tools/shared/world/WorldNode.cpp | 31 | 28 | 3 | 0 | 0 |
| 0x0046f3c0..0x0046fec3 | DRAGDOCK | tools/DEdit/MRCEXT/DRAGDOCK.CPP | 21 | 18 | 1 | 2 | 0 |
| 0x0046fec3..0x0047065c | FRAMEWND | tools/DEdit/MRCEXT/FRAMEWND.CPP | 24 | 18 | 0 | 6 | 0 |
| 0x0047065c..0x004709d5 | MDIFLOAT | tools/DEdit/MRCEXT/MDIFLOAT.CPP | 15 | 13 | 0 | 2 | 0 |
| 0x004709d5..0x00471982 | SIZECONT | tools/DEdit/MRCEXT/SIZECONT.CPP | 44 | 27 | 15 | 2 | 0 |
| 0x00471982..0x00471c8a | SIZEDLG | tools/DEdit/MRCEXT/SIZEDLG.CPP | 15 | 8 | 2 | 5 | 0 |
| 0x00471c8a..0x00473b63 | SIZEDOCK | tools/DEdit/MRCEXT/SIZEDOCK.CPP | 55 | 48 | 4 | 3 | 0 |
| 0x00473b63..0x0047461e | SZTOOLBA | tools/DEdit/MRCEXT/SZTOOLBA.CPP | 31 | 10 | 13 | 3 | 5 |
| 0x0047461e..0x00475dd1 | RVTracker | tools/DEdit/Trackers/RVTracker.cpp | 45 | 12 | 25 | 2 | 6 |
| 0x00475dd1..0x0047a683 | RVTrackerDrawPoly | tools/DEdit/Trackers/RVTrackerDrawPoly.cpp | 99 | 20 | 70 | 5 | 4 |
| 0x0047a683..0x0047cdba | RVTrackerTextWrap | tools/DEdit/Trackers/RVTrackerTextWrap.cpp | 45 | 11 | 30 | 2 | 2 |
| 0x0047cdba..0x0047d040 | TrackerMgr | tools/DEdit/Trackers/TrackerMgr.cpp | 14 | 8 | 5 | 1 | 0 |
| 0x00483020..0x004835a0 | GenRegMgr (2001) | lithshared/genregmgr (genregmgr60_Talon.dsp) | 12 | 12 | 0 | 0 | 0 |
| 0x004835a0..0x00488ed0 | ButeMgr | ../Libs/ButeMgr (ButeMgr_Talon.dsp) | 137 | 132 | 2 | 2 | 1 |
| 0x00488ed0..0x00489190 | RegMgr (LT2) | lithshared/regmgr (regmgr60_Talon.dsp) | 7 | 7 | 0 | 0 | 0 |
| 0x00489190..0x0048b5e0 | StdLith (LT2, allocator override) | lithshared/stdlith (Lib_StdLith_alloc.dsp) | 94 | 84 | 1 | 9 | 0 |
| 0x0048b5e0..0x0048ba90 | DibMgr (2001) | lithshared/dibmgr (dibmgr60_Talon.dsp) | 12 | 12 | 0 | 0 | 0 |
| 0x0048ba90..0x0048d740 | the LTA library | ../Engine/libs/ltamgr (ltamgr_Talon.dsp) | 77 | 55 | 16 | 4 | 2 |
| 0x004a25f0..0x004a7d00 | zlib 1.0.4 | lithshared/zlib (Lib_ZLib_Talon.dsp) | 57 | 57 | 0 | 0 | 0 |

What the table says, where Build 69 doesn't:

- **The Project Bar group** comes first (BaseImgDlg and ModelDlg have no
  function of their own that matches alone: 0x401000..0x401f04 is both,
  0x40189f is ModelDlg's "\\Models\\*").
- **concommand and DebugDlg are 2001's** (0x4145a8, 12 functions, and
  0x41640c, before DEdit): the project still has them in the "Build 69
  only" group, out of their place by name. Moving them costs five small
  identical functions to the link's folding (0x443fd2, 0x470f93,
  0x4756cb, 0x48cc10, 0x48d360), so they wait for the matching round.
- **LTTexture's** texture dump functions ("\\nTextureMipData {", ...)
  match at 0x461fd5..0x4624a7, inside the Texture module (Texture.cpp,
  which includes TextureHelper.cpp): 2001's Texture.cpp has them.
- **The model code is ModelEdit's 2001 code**: model.cpp 226 of 267
  identical, model_load.cpp 83 of 93, as they are in tools/shared/model.
  modelcallbacks.cpp's three matches are at 0x43c559, after NewProjectDlg,
  out of order: its 2001 name is not "modelcallbacks".
- **The libraries** are nearly done: GenRegMgr, RegMgr, DibMgr and zlib all
  identical, ButeMgr 132 of 137, StdLith 84 of 94, the LTA library 55 of 77.
- **flattoolbar.obj** matches nafxcw.lib's CToolBar code (0x4b6784..): it
  is MFC's bartool.cpp borrowed; 2001's position for it is unknown.
- Not found by any object of ours (2001-only, or Build 69's too far from
  it): the Quake .map exporter (0x427ff0, after EditRegion), the old model
  dims reader (0x42da61, before MainFrm), the light animation writer
  (0x450c20, after RegionDoc), the primitive creation ("Cylinder
  Primitive", "Pyramid Primitive", 0x46af92..0x46b404, after
  ViewFunctions: Build 69's CreatePrimitive.cpp is elsewhere by name).

## Strings, by function (build/strmap.py)

Address order; strings of three characters and noise left out.

```
0x0040144c    604  %s\\shell32.dll | Name | Size | Modified
0x004017a2    234  open
0x0040189f    786  \\Models\\*
0x004025d3    142  Unhiding all nodes... | Ready
0x00402661    143  Hiding all nodes... | Ready
0x004028df    242  Container | Unknown
0x004046f6     42  %s-ProjectBar
0x00404720     69  %s-ProjectBar
0x004048b0    194  .ABC
0x00404bb9    176  Courier New
0x00404ee8   1419  Color | True
0x004056c8    431  Information not available.  Please check the classhlp.but file and res
0x00405a4a    637  True | False
0x00405d23   2282  True | False | Error: The %s object cannot be found. | Error
0x00406734    606  All files (*.*)|*.*|Model (*.abc)|*.abc|Texture (*.dtx)|*.dtx|Wave (*. | Error | Error: The file that you picked is not in the resource path for th...
0x00406dfa    176  Courier New
0x00406f60    538  ClassDescription
0x0040724f    363  Change Class
0x004079ee    604  %s\\shell32.dll | Name | Size | Modified
0x00407cf9   1005  open | \\Sounds\\*
0x00408209    604  %s\\shell32.dll | Name | Size | Modified
0x00408d5d    569  %s\\shell32.dll | Name | Size
0x0040979b   2115  \\Textures\\*
0x00409fe9   3387  .dtx | DTX Files (*.dtx)|*.dtx||
0x0040ad24   1692  LIGHTDEFS
0x0040b3c0   2021  \\Textures\\*
0x0040d065    644  %s\\shell32.dll | Name | Size | Modified
0x0040d4d8    318  Are you sure you want to delete the world  | Confirmation | Error | Internal error deleting world! | Error deleting world
0x0040d6b4    133  Selected %d objects | Done
0x0040d739     77  Un-Selected %d objects
0x0040d786     32  Done
0x0040e7fb    137  Texture Palette "%s"%s
0x0040e8b0    650  *.dtx
0x0040eb95    148  The palette "%s" has not been saved!  Would you like to save it now? | Confirmation
0x0040eda1    255  Palette Files (*.pal)|*.pal|| | *.pal
0x0040eea0    286  .pal | Palette Files (*.pal)|*.pal|| | *.pal
0x0040f017    234  Unnamed
0x0040f650     96  TextureWrap.Style | TextureWrap.Offset | TextureWrap.Scale | TextureWrap.RestrictDir
0x0040f6b0     82  TextureWrap.Style | TextureWrap.Offset | TextureWrap.Scale | TextureWrap.RestrictDir
0x0041110a    303  %dKB
0x00411735    137  ObjectDLLSetup
0x004122f6    204  Name
0x00412bc4    256  \ng default | \ng object
0x00413037     51  CustomColors
0x00413078     42  CustomColors
0x00414f16    102  Primitive
0x00415181     68  Couldn't find a supported texture format!
0x00415330     70  CURRENT - 
0x00415376     59  %s%s - %d bits (%d %d %d %d) (%x %x %x %x)
0x004153b1     81  DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXEDTO8 | DDPF_ALPHAPIXELS | DDPF_RGB | ...
0x0041653e     37  DEdit
0x004166da    146  LithTech Inc.\\Talon
0x0041676e    321  ClassHlp.but
0x00416989     26  noundo
0x00416b48    551  Project
0x00416d6f    256  Worlds | Sprites | Textures | Sounds | Models
0x00417478    246  Auto-saving: 
0x0041756e     66  Ready
0x004175d5     54  HKEY_CURRENT_USER\\Software\\LithTech Inc.\\Talon\\DEdit
0x00417626     36  http://www.lithtech.com | open
0x0041764a     79  %s\\lithtech.hlp | open
0x00417a36    654  \\Worlds\\*
0x00417d60    150  | - BRUSH | - OBJECT | - NULL | %s%s: Class '%s', Name '%s', ID: '%d'
0x00417e02    281  Not enough texture arguments specified.  Please use the form: |   ReplaceTex CurrentTexture NewTexture | No active document available. | No active ...
0x0041825c    566  \\OptionsDisplay | \\AdvancedSelect | \\ClassDlg | \\Primitives | \\Run | \\Warnings | \\ObjectBrowser | \\GenerateUniqueNames | \\Clipboard | \\Wi...
0x00418492     53  AutoSaveTime
0x004184c7     49  AutoSaveTime
0x00418eac   1495  Couldn't find device %d | DirectDrawCreate failed. | QueryInterface(IID_IDirectDraw4) failed. | SetCooperativeLevel failed. | QueryInterface(IID_ID...
0x00419483    251  Chose d3d driver %s, description %s
0x0041f7b4    638  dtx_Create
0x0041fa96    421  dtx_Save | Invalid size | LT_UNSUPPORTED
0x004236e3     16  DUMMY
0x0042462e     17  DEditTransfer
0x00424d06     86  Default
0x00424e7f    431  surf_xzonly | spriteanimate
0x00425bb7    589  object.lto | Software | LithTech Inc. | Demo | default_app_dlls
0x00426047    254  lithtech.cls
0x00426b9d   1000  bool | color | flags | longint | real | rotation | string | vector | beziernexttangent | bezierprevtangent | dims | dynamiclist | fieldofview | fil...
0x00426fc2    959  header | infostring | polyhedronlist | globalproplist | nodehierarchy | worldnode | navigatorposlist
0x00427a91    257  name | propid
0x00427b92    836  childlist | null | brush | object | properties | flags | selected | hidden | path | worldroot | expanded | label
0x00427ff0    775  "classname" "worldspawn"\n | "wad" "medieval.wad"\n | StartPoint | "classname" "info_player_start"\n | m_Radius | "classname" "light"\n | "light" "...
0x0042b5ee     31  DEdit Palette File
0x0042b6c5    362  Error opening palette file %s. | Error | Invalid palette file %s: Invalid file header.
0x0042b82f    264  Error opening palette file %s. | Error
0x0042bf7a    126  true | false
0x0042da61    482  Header | MonolithExport Model File v6 | Dimensions | Animation
0x0042e447   1427  General
0x0042edff    631  Settings | DEdit | LithTech Inc.\\Talon | MainWindow | ToolTips | ColorSelectDlg | DScriptEditor | Undo Preferences
0x0042f07b    487  Settings | DEdit | LithTech Inc.\\Talon | MainWindow | ToolTips | ColorSelectDlg | DScriptEditor | Undo Preferences
0x0042f331    901  Textures for world %s | (Invalid) | %dx%d count: %d
0x0042f75c    301  Classes for world %s | (Invalid)
0x0042f9cd    149  DEdit
0x0042fbe3    551  .rez | There is not enough free disk space available on target drive.
0x0042fe70    313  General
0x00430765    854  .dat | Cannot find file | Error: Cannot find the .dat file for your world.  Please run the proce | %WorldName% | Error: Cannot change working direc...
0x004310e9    521  Couldn't open %s.
0x0043353f   1041  VisRadius | AmbientLight | DirLight | ShadowProjectLength | ShadowLightDist | ShadowSizeX | ShadowSizeY | ShadowCenterOffset | FadeSpriteSize | Nor...
0x00437e8c    271  AnimBindings
0x00437f9b    229  Sockets
0x00438124   2404  Pieces | Nodes | ChildModels | SELF | Animation
0x00438acc     88  Header
0x00439c12     51  basic_string
0x0043b08d    150  (Read-only)
0x0043b2a4    237  Original Name | Updated Name
0x0043b3b2    300  Property Name | Original Value | Updated Value
0x0043b71c    417  forward | right | position | lookat | lookatdist
0x0043bbd8    278  navigator | description
0x0043c5bd     94  Brush | BrushType | FrictionCoefficient
0x0043d041    251  Node View | Class types cannot be changed in the node view.
0x0043deae    549  Object | Object %s (%s) has class type collision with %s.  Would you like to sk | Warning | Unable to create object of class 
0x0043e6a8    233  UseNameField | MatchWholeName | NameField | UseClassField | ClassField | SelectionOperation | ShowResults
0x0043e791    111  UseNameField | MatchWholeName | NameField | UseClassField | ClassField | SelectionOperation | ShowResults
0x0043eb18    268  NumRecentClasses | RecentClass%d | MaxNumRecentClasses | ShowTree | BindIndividually
0x0043ec27    167  NumRecentClasses | RecentClass%d | MaxNumRecentClasses | ShowTree | BindIndividually
0x0043ed14     63  GenerateUniqueNames | UpdateReferencingProperties | DisplayNameChangeReport
0x0043ed53     51  GenerateUniqueNames | UpdateReferencingProperties | DisplayNameChangeReport
0x0043f03f   2477  Background | ColorBackground | MajorGrid | ColorMajorGrid | MinorGrid | ColorMinorGrid | Brush | ColorBrush | Selected Brush | ColorSelectedBrush |...
0x0043fabd    228  ShadePolygons | ShowSurfaceColor | VertexSize | HandleSize | VertexDrawRule | UseD3DAsDefault | DefaultD3DDevice | DefaultD3DMode | ZBufferLines
0x0043fba1    214  ShadePolygons | ShowSurfaceColor | VertexSize | HandleSize | VertexDrawRule | ZBufferLines | UseD3DAsDefault | DefaultD3DDevice | DefaultD3DMode | ...
0x0043fcf2     63  UpdateReferencingProperties | UpdatedSelectedPropertiesOnly | DisplayReportOfChanges
0x0043fd31     51  UpdateReferencingProperties | UpdatedSelectedPropertiesOnly | DisplayReportOfChanges
0x0043fda4     25  GroupByType
0x0043fdbd     17  GroupByType
0x0043fe07     67  Sphere | Dome
0x0043fe4a     66  Sphere | Dome
0x0043fe8c    204  NumSides | VerticalDivisions | Radius
0x0043ff58    168  NumSides | VerticalDivisions | Radius
0x004400d5    214  lithtech.exe | ExecutablePath | ..\\Game | WorkingDirectory | -rez AVP2 +runworld %WorldName% | ProgramArguments
0x004401ab     51  ExecutablePath | WorkingDirectory | ProgramArguments
0x004402d8    413  NumMainProjectControls | MainProjectControl%d | NumBarProjectControls | BarProjectControl%d
0x00440475    220  NumMainProjectControls | MainProjectControl%d | NumBarProjectControls | BarProjectControl%d
0x00443118    100  Courier New
0x004435ce    644  %s\\shell32.dll | Name | Size | Modified
0x004442ce    247  Processor  | -bsp  | -light  | -optimize  | -fast  | -unseen  | -sky 
0x00444af3     82  textures\\default.dtx
0x0044d4b2     48  data
0x0044d603    109  vector | data
0x0044d78f    109  eulerangles | data
0x0044d913     41  data
0x0044d9e4     46  data
0x0044e49b    127  Do you want to reset your colors to the default settings? | Reset
0x0044e880    302  Hardware Acceleration | Software MMX | Software 8-bit Emulation | Software RGB Emulation
0x0044edbe    177  Executable (*.exe)|*.exe||
0x0044f79e    630  .obj
0x0044fa6a    329  "%s\\Processor" "%s"
0x0045035a   1035  Zone | Unknown prop name: %s | Unknown prop type: %s | Unknown prop flags: %s | Unmatchable string for static list: %s
0x00450c20    420  LightAnim_BASE
0x00452bb7   4830  Select.Depth | ApplyTexture | SplitBrush | CarveBrush | Select.None | SetGridDir.Forward | SetGridDir.Right | SetGridDir.Up | InitTextureSpace | De...
0x00453e95   4496  EditLightNode.Down | NodeRotate | TextureMove | TextureWrap | BrushSize | CurveEdit | FarDist | GridSize | MarkerMove | VertMove.Poly | VertMove.Se...
0x00455045      6  CUIKeyEvent
0x0045504b      6  CUIEvent
0x0045516c      6  CUIMouseEvent
0x00456467    255  \\*.txt | Text (*.txt)|*.txt||
0x00456566    112  DEdit World (*.ed)|*.ed|Obj File (*.obj)|*.obj|LTA Geometry (*.lta)|*.
0x004565d6    310  .lta | .obj | Error: Unknown extension on selected file
0x0045670c    498  Error | Unknown error | Error: Invalid file version | Error: Out of memory | Error: Invalid file
0x004568fe    579  Error | Error: Unable to open file | Unknown error | Error: Invalid file version | Error: Out of memory | Error: Invalid file
0x00456b41   1361  Brush | Terrain | \ncreating brushes | NoSnap | LightMap | Subdivide
0x00457092    351  \ncalling objInterface.ImportObj() | \ncalling CreateBrushesFromScene() | \ncalling region.SaveFile(file); | \ncalling ImportWorldFile
0x00458a53    474  m_Pos
0x00458eb5    235  .map
0x0045915c   1299  Please remove any neighboring coplanar polygons before performing this | Invalid geometry has been detected on this brush.  Please clean up the
0x0045a56d     69  DetailLevel
0x0045a5b2    449  POS(%.1f, %.1f, %.1f)
0x0045ad15    138  DetailLevel
0x0045bfc4    547  \\Prefab\\*.ed | DEdit World (*.ed)|*.ed|| | Error | Error: Unable to open file
0x0045c28a    531  *.lek | Leak Files (*.lek)|*.lek|All Files (*.*)|*.*|| | Error: Cannot open  |  for reading. | Error
0x0045c6a1    238  Invalid Value | Please enter a non-zero floating point value.
0x0045cc13    457  Zone
0x0045f14a    589  DirTypeModels | DirTypeScripts | DirTypeSounds | DirTypeSprites | DirTypeTextures | DirTypeWorlds | DirTypeProject | DirTypePrefabs
0x0045f417    140  DirTypePrefabs | DirTypeModels | DirTypeSounds | DirTypeTextures | DirTypeSprites | DirTypeWorlds | DirTypeProject
0x004617dd     51  Internal error in texture list!
0x00461826    754  Unable to open texture file: %s. | dtx_Create() for texture %s failed. | PALLETE32 | CDibMgr::AddDib() for texture %s failed.
0x00461e9e    311  \nTextureMipData { | \n\t m_Width %d m_Height %d | \n\t m_Data %x | \n\t m_Pitch %d | \n\t m_BumpData %d | \n\t m_BumpPitch %d | \n\t color_data: \n
0x004624a7    718  PALLETE32
0x004627b4    462  TexturePreview | Preview
0x00462a45    913  Converting... | PALLETE32 | Compressing... | Decompressing... | Drawing...
0x004689ba   1200  Can't flip without a selected brush
0x0046a13c    407  \\Textures\\*
0x0046af92    186  Cylinder Primitive
0x0046b404    186  Pyramid Primitive
0x00470007     59  Version
0x00470042    115  Version
0x004700b5    504  BarSize_
0x004709d5    326  Marlett
0x004717dd    339  AfxControlBar
0x00471982    226  ComboBox
0x00483020     29  True | False
0x00483060    112  TRUE
0x00483260    706  HKEY_CLASSES_ROOT | HKEY_CURRENT_USER | HKEY_LOCAL_MACHINE | HKEY_USERS
0x004835a0    425  FALSE | TRUE
0x00484610    500  Bad symbol encountered.
0x00484848    155  File "%s", line %d :  %s | Line %d :  %s
0x004848f0     55  A formatting error in the attribute file was encountered
0x00484930    383  duplicate symbol encountered - %s
0x00484aaf    716  (DWORD) | (BYTE) | TRUE | FALSE | (FLOAT)
0x00485355    375  Invalid token encountered.
0x004854d0     85  A formatting error in the attribute file was encountered.
0x00485530     82  Duplicate tag encountered - %s
0x004855a0    872  (DWORD) | (BYTE) | TRUE | FALSE
0x004860da     30  Type mismatch - [%s]:%s
0x00486100     60  Symbol not found - [%s]:%s
0x00486140     74  Type mismatch - [%s]:%s
0x00486190     57  Symbol not found - [%s]:%s
0x004861d0     72  Type mismatch - [%s]:%s
0x00486220     57  Symbol not found - [%s]:%s
0x00486260     72  Type mismatch - [%s]:%s
0x004862b0     57  Symbol not found - [%s]:%s
0x004862f0     86  Type mismatch - [%s]:%s
0x00486350     62  Symbol not found - [%s]:%s
0x00486390     86  Type mismatch - [%s]:%s
0x004863f0     72  Symbol not found - [%s]:%s
0x00486482     45  Type mismatch - [%s]:%s
0x004864b0    179  Symbol not found - [%s]:%s
0x0048659a     30  Type mismatch - [%s]:%s
0x004865c0    119  Symbol not found - [%s]:%s
0x0048667a     30  Type mismatch - [%s]:%s
0x004866a0    109  Symbol not found - [%s]:%s
0x0048674a     30  Type mismatch - [%s]:%s
0x00486770    157  Symbol not found - [%s]:%s
0x0048684a     30  Type mismatch - [%s]:%s
0x00486870    137  Symbol not found - [%s]:%s
0x00488ed0    249  Software | ware
```

## Build 69 and the 2001 image

Build 69 (2006) added, and the image hasn't (none of their strings is
there):

- the level-error checker (LevelError/*: "Concave Poly",
  "DefaultErrorDetector", "%d Level Errors"; its dialogs);
- prefabs as references: PrefabMgr, PrefabRef, the prefab bar and tracker
  and their options ("Unable to create prefab reference object from
  filename"). The 2001 DEdit's prefabs are worlds: RegionView saves a
  selection into "\\Prefab\\*.ed" (0x0045bfc4) and the resource manager has
  a DirTypePrefabs directory, and the 2001 CEditRegion has no prefab manager
  (the processor's is 0x128 bytes);
- the second texture of a poly (Build 69's CTexturedPlane m_Textures[2]:
  detail textures, effect groups, EditEffectGroupDlg, SelectEffectGroupDlg)
  and per-vertex colour; the 2001 poly has one texture (O, P, Q and its
  name in CEditPoly, the processor's layout);
- bump and cube maps (ImportBumpMapDlg, ImportCubeMapDlg, "CUBEMAPDATA"),
  the level items and level textures dialogs, light attenuation presets,
  the texture and object trackers' CSV export, object selection filters,
  the event editor and list (PF_EVENT), the rename-resource and
  update-all-objects tools, the undo warning, the debug dialog, the folder
  browser (DirDialog: no SHBrowseForFolder import);
- models drawn in the views (ModelView: the model manager, mesh shapes,
  PF_MODEL), and the property flags PF_ORTHOFRUSTUM, PF_DISTANCE,
  PF_COMPOSITETYPE, PF_TEXTUREEFFECT and PF_NOTIFYCHANGE with
  IObjectPlugin::PreHook_PropChanged ("[10/01/01 ARP]": just after AvP2):
  the image's LTA property flags are the LT2 SDK's eighteen (staticlist ..
  beziernexttangent, 0x004f808c..);
- the hotkey configuration (keyconfigdlg, the registry's "%sList%dEvent%dType",
  "NumHotKeys") and the misc, models, controls and viewport options pages'
  registry values ("AutoLoadProj", "ControlStyle", "ShadeMode", ...);
- TdGuard, and the rez file written through Engine/libs/rezmgr's
  RezCompiler (the image's OnCreateRezFile, 0x0042fbe3, asks no
  confirmation and calls a function of its own module, 0x0042fe0a).

The image has, and Build 69 hasn't: a Quake .map exporter
("\"classname\" \"worldspawn\"", "\"wad\" \"medieval.wad\"", 0x00427ff0, between
EditRegion and ExportObjFile by name), an old model reader for object
dims ("MonolithExport Model File v6", "Dimensions", "Animation", 0x0042da61,
between load_pcx and MainFrm), light animations saved with the region
("LightAnim_BASE", 0x00450c20, after RegionDoc), and the 2001 model code
ModelEdit has (model.cpp's command keys "VisRadius", "AmbientLight", ...
at 0x0043353f, the .abc loader's chunk names at 0x00437e8c..0x00438acc:
ModelEdit's model.obj, as it is, matches 387 of the image's functions,
model_load.obj 78, model_cleanup.obj 37). DEdit's world code is the
processor's 2001 world code, compiled with DIRECTEDITOR_BUILD: its
classes are the 2001 ones (CEditVert 40 bytes, CEditPoly one texture,
CEditBrush with a bounding box, CWorldNode's five virtuals), with the
editor's members besides; the processor's objects (cl 8966 /O2) don't
match DEdit's (9044 /O1) byte for byte, so it is a guide to shapes, not a
source of identical functions.

What the project has of Build 69's that the image hasn't (the "Build 69
only" group, linked so that the rest links), and the stand-ins (marked
`talon: stand-in` in the sources): the Build 69 only group (45 sources: the level-error checker, the
prefab manager, references, bar, tracker and options, effect groups, bump
and cube maps, the level items and textures dialogs, light attenuation,
the folder browser, update-all-objects, the undo warning, the texture
tracker, object selection filters, the event editor and list, the
hotkey configuration, the model view, the misc, models, controls and
viewport options, rename resource, LTTexture, and concommand and DebugDlg
(2001's, above)); and the stand-ins: Build 69's vertex flags and colour in
CEditVert, its two textured planes in CEditPoly and its prefab manager in
CEditRegion (shared/world, under DIRECTEDITOR_BUILD), an empty
CBaseEditObj::OnPropertyChanged, a local IsFileAbsolute (EditObjects,
ProjectBar), the brush culling sphere made from the 2001 bounding box
(DrawBase), no RezCompiler in MainFrame::OnCreateRezFile, and a model
loader that loads nothing (ModelView/modelmgr.cpp).
