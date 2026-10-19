LithTech Jupiter tools, built from Build 69's source
====================================================

The editing tools for No One Lives Forever 2, Contract J.A.C.K. and
Tron 2.0, built from the source in Touchdown Entertainment's 2006
"Jupiter System Enterprise Edition, Build 69" by lithtools
(jupiter/README.md in the repository says how). This is a test build:
it is meant to be tried, not yet relied on.

What is here
------------

The layout is Build 69's own Tools folder. MANIFEST.txt says where every
file came from: "built" files were compiled from source, the rest (data
files and the Microsoft runtime DLLs) are Build 69's own.

  dedit.exe               world editor
  ModelEdit.exe           model editor
  winpacker.exe           world packer (packers\WorldPacker.dll)
  Model_Packer.exe        .lta -> .ltb model packer (command line)
  LithRez.exe             .rez archive tool (command line)
  FxED.exe                ClientFX editor
  RenderStylesEditor.exe, RenderStyle_Packer.exe
  ButeEdit.exe, WaveEdit.exe, LTC.exe, WinLTC.exe, lipcompiler.exe,
  dtxutil.exe, DtxView.exe, DtxOptimizer.exe, DtxShellEx.dll, ...
  Plugins\3dsmax7\plugins\          3ds Max 7 exporters/importer/DTX loader
  Plugins\alias\maya6.5, maya7.0\   Maya model and world exporters
  Plugins\...\image\IMFLithDTX.dll  Maya DTX image plug-in
  Plugins\PhotoShop\DTXFormat.8bi   Photoshop DTX format plug-in

Not included: plug-ins for 3ds Max 3-6 and Maya 4.0-6.0 (their SDKs are
not available), and Build 69's ButeCheck, CRUD, xls2rc and ServerDir.dll,
which are not part of its tools source.

Differences from Build 69's own binaries
----------------------------------------

- Model_Packer writes model file version 23 by default, as the NOLF2,
  Contract J.A.C.K. and Tron 2.0 engines expect (Build 69 writes 25,
  whose OBB records those engines misread). "-ModelVersion 25" writes
  Build 69's version. With version 23 its output is byte for byte what
  Contract J.A.C.K.'s own Model_Packer writes.
- dedit.exe is the "Release Public Development" build, like the public
  tools of NOLF2 and Contract J.A.C.K.: its default game executable is
  LithTechDev.exe.

Running under Wine
------------------

Any 32-bit-capable Wine. d3dx9_29.dll is Wine's own; nothing needs
installing. For example:

  wine dedit.exe
  wine Model_Packer.exe d3d -input model.lta -output model.ltb
  wine LithRez.exe v Game.rez .

(LithRez's "v" needs a third argument, any will do: an old bug in it.)

On Windows the DirectX 9 runtime (February 2006 or later, for
d3dx9_29.dll) is needed.

Point DEdit's project at a game's Development folder as the NOLF2 or
Contract J.A.C.K. toolkits set it up (Game\ with the game's .lta/.ltc
worlds, prefabs, textures and the game's object.lto for its object
classes).

Terms
-----

Monolith Productions' / LithTech, Inc.'s code, later Touchdown
Entertainment's; see Legal notices in the source. Do not redistribute
this build until its terms are worked out.
