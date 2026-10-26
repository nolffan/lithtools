"""What the scoreboard scores: the Talon tools' shipped binaries, and where
each one's own code, and ours, is.

tools/progress.py and tools/ratchet.py read this file and nothing else
about the project, so they know no binary by name: a binary is added here,
not in the tools. Plain data, with the evidence for every address beside
it. The keys are cjacksrc's (progress/targets.py there documents them):
label, what, kind, shipped, stem, built, optional, app, middleware,
third_party, cites, cited_src, purecall.

The shipped binaries come from build/reference.sh: the AvP2 Editing Tools
v1.0, Update 1 and Update 2 off the Primal Hunt disc, and NOLF Tools 001.
How the ranges were found: `tools/libscan.py <image> <lib>...` puts each
library's functions in the image (msvc-6.00-8966's msvcrt.lib, mfc42.lib,
libcmt.lib, nafxcw.lib, libcimt.lib); the image's own code ends where the
runtime's first member starts. Pinned from our .map where we have a link.
"""

REFERENCE = "out/reference"     # build/reference.sh writes it
OUT = "out/progress"            # each run's functions.csv and summary.json
DATA = "progress"               # the recorded baseline and the evidence

CITED_SRC = []
LIBS = []
SHIPPED_LIB = None
SHIPPED_AS = {}
SHIPPED_ENV = None
DRIVER_OBJS = set()

U2 = REFERENCE + "/avp2tools_update2/Tools"
AVP2 = REFERENCE + "/avp2tools/Tools"
NOLF = REFERENCE + "/nolftools001/Tools"

TARGETS = [
    # ---- the editors of AvP2 Tools Update 1 and 2 ("Talon", 2001-12) ---
    # Update 2 carries Update 1's DEdit and ModelEdit unchanged (same
    # SHA-256). Both are cl 12.00.9044 (msvc-6.00-9044) with MFC linked
    # statically (nafxcw.lib, build 8569) and the old iostreams
    # (libcimt.lib). Provisional ranges: to the first static MFC member
    # libscan.py finds (wincore.obj's _InitMultipleMonitorStubs); the
    # shared libraries (ButeMgr, StdLith, RegMgr, DibMgr) are below it,
    # as Monolith code of ours to build.
    dict(label="DEdit.exe", what="world editor (Talon, Update 1/2)", kind="image",
         shipped=U2 + "/DEdit.exe", stem="DEdit", built=None, optional=True,
         app=[(0x00401000, 0x0048d759)]),
    dict(label="ModelEdit.exe", what="model editor (Talon, Update 1/2)", kind="image",
         shipped=U2 + "/ModelEdit.exe", stem="ModelEdit", built=None, optional=True,
         app=[(0x00401000, 0x00437194)]),
    # ---- the world processors ------------------------------------------
    # AvP2 v1.0's (2001-10-03, cl 12.00.8966) writes AvP2's world version
    # 70; NOLF Tools 001's (2001-01-03, cl 12.00.8447) NOLF's 66
    # (FORMATS.md). Static MFC; provisional ranges as the editors'.
    dict(label="Processor.exe (AvP2)", what="world processor, AvP2 (v1.0)", kind="image",
         shipped=AVP2 + "/Processor.exe", stem="Processor-avp2",
         # src/LT2/tools/PreProcessor/Processor.dsw, built with
         # WITH_PROCESSOR=1 while its sources do not all compile. Its own
         # code is followed by the LT2 libraries (StdLith 0x0044eab0, zlib
         # 0x00450ad0, RegMgr 0x004564e0..0x00456742: Monolith's, ours to
         # build, so inside the range) and from 0x0045680f static MFC
         # (progress/Processor-avp2-map.md)
         built="src/LT2/built/Release/tools/Processor/Processor.exe", optional=True,
         # until it links, its compiled objects are scored on their own,
         # with the LT2 libraries its link will read
         partial_objs=["src/LT2/built/Release/tools/Processor/*.obj",
                       "src/LT2/lithshared/built/Release/Lib_StdLith/Lib_StdLith.lib",
                       "src/LT2/lithshared/built/Release/Lib_ZLib/Lib_ZLib.lib",
                       "src/LT2/lithshared/built/Release/RegMgr/regmgr.lib"],
         app=[(0x00401000, 0x0045680f)]),
    dict(label="Processor.exe (NOLF)", what="world processor, NOLF (Tools 001)", kind="image",
         shipped=NOLF + "/Processor.exe", stem="Processor-nolf", built=None, optional=True,
         app=[(0x00401000, 0x00469103)]),
    # ---- the resource packer ---------------------------------------------
    # LithRez 1.00 (Nov-6-1998), shipped unchanged in AvP2 Tools v1.0 and
    # NOLF Tools 001 (same SHA-256). Linker 5.10 and no Rich header: Visual
    # C++ 5.0; the C runtime static (libc, only KERNEL32 imported). Build
    # 69 has its source (tools/LithRez, libs/rezmgr). The link: lithrez.obj
    # (0x00401000..0x00402dc0; 0x00402dc0 is the runtime's printf), the C
    # runtime members it pulls in, then RezMgr's objects (rezmgr, rezhash,
    # rezfile) and Lith's (basehash, baselist, virtlist), 0x0040ad50..
    # 0x0040ea90; after them the runtime's _stricmp, operator new and
    # delete), more runtime, and at the end of .text the C++ unwind
    # funclets (0x0040fee0..0x004101e5, jumping into lithrez.obj and RezMgr).
    # lithrez.obj is lithrez.cpp then rezutils.cpp (0x004011e0).
    dict(label="lithrez.exe", what="resource (.rez) packer, LithRez 1.00", kind="image",
         shipped=AVP2 + "/lithrez.exe", stem="lithrez",
         # src/LT2/tools/LithRez/LithRez.dsw (Visual C++ 5.0 SP3, build.mk's VC5)
         built="src/LT2/built/Release/tools/LithRez/LithRez.exe", optional=True,
         partial_objs=["src/LT2/built/Release/tools/LithRez/*.obj",
                       "src/LT2/built/Release/tools/LithRez/RezMgr/*.obj",
                       "src/LT2/built/Release/tools/LithRez/Lith/*.obj"],
         # (the kernel32 thunk at 0x0040e7d0 and the unwind funclets at
         # 0x0040fee0..0x004101e5 are the runtime's kind: not counted)
         app=[(0x00401000, 0x00402dc0), (0x0040ad50, 0x0040e7d0),
              (0x0040e7e0, 0x0040ea90)]),
    # ---- DirectEngine's world processor (NOLF Tools 001) -----------------
    # "--- DirectEngine World Processor ---", built 1998-05-31 from
    # F:\Proj\DE\DEdit\TProcessor: the LithTech 1.0-era processor that
    # NOLF Tools 001 still ships. Linker 5.10, no Rich header: Visual C++ 5.0
    # (msvc-5.00-7303), with the multithreaded C runtime (libcmt: TlsAlloc
    # imported) linked statically. libscan.py with that libcmt.lib puts the
    # runtime from 0x0043b2f0 (exsup.obj's __global_unwind2) to 0x0044b9a0,
    # and getch/putch at 0x00451910; zlib 1.0.4 is below it, ours to build.
    dict(label="TProcessor.exe", what="DirectEngine world processor (1998, NOLF Tools 001)", kind="image",
         shipped=NOLF + "/TProcessor.exe", stem="TProcessor", built=None, optional=True,
         app=[(0x00401000, 0x0043b2f0)]),
    # ---- the Softimage|3D and Maya exporters of AvP2 v1.0 and NOLF ------
    # LithExport.dll (models) and LandExport.dll (terrain) are Softimage|3D
    # plug-ins (they import SAA.dll), link 6.00, 2000-08 (AvP2 v1.0) and
    # 2000-09 (NOLF Tools 001); the C runtime static. NOLF's LithExport is a
    # debug build (libcmtd: its asserts name d:\proj\nolf\lt2\tools\model\
    # softexport, and _chkesp starts the runtime). LithMayaExport.mll is a
    # Maya (1.x/2.0) plug-in of 1999-02. Building any of them needs that
    # package's SDK of the time, which we don't have yet. Ranges: to the
    # runtime's first member libscan.py finds with msvc-6.00-8447's libcmt
    # (libcmtd for NOLF's LithExport).
    dict(label="LithExport.dll (AvP2)", what="Softimage|3D model exporter (AvP2 v1.0)", kind="image",
         shipped=AVP2 + "/Exporters/LithExport.dll", stem="LithExport-avp2", built=None, optional=True,
         app=[(0x10001000, 0x100290d0)]),
    dict(label="LithExport.dll (NOLF)", what="Softimage|3D model exporter, debug build (NOLF Tools 001)", kind="image",
         shipped=NOLF + "/Exporters/LithExport.dll", stem="LithExport-nolf", built=None, optional=True,
         app=[(0x10001000, 0x10057800)]),
    dict(label="LandExport.dll (AvP2)", what="Softimage|3D terrain exporter (AvP2 v1.0)", kind="image",
         shipped=AVP2 + "/Exporters/LandExport.dll", stem="LandExport-avp2", built=None, optional=True,
         app=[(0x10001000, 0x10013950)]),
    dict(label="LandExport.dll (NOLF)", what="Softimage|3D terrain exporter (NOLF Tools 001)", kind="image",
         shipped=NOLF + "/Exporters/LandExport.dll", stem="LandExport-nolf", built=None, optional=True,
         app=[(0x10001000, 0x10013810)]),
    # The runtime's first member is trnsctrl.obj (0x1000a949); the small
    # functions from 0x1000a930 look like its own helpers: provisional.
    dict(label="LithMayaExport.mll", what="Maya model exporter (1999, AvP2 v1.0)", kind="image",
         shipped=AVP2 + "/Exporters/LithMayaExport.mll", stem="LithMayaExport", built=None, optional=True,
         app=[(0x10001000, 0x1000a930)]),
    # ---- the plug-ins of AvP2 Tools Update 2 (built 2002-03-28) ----------
    dict(label="IMFLithDTX.dll", what="Maya .dtx image plug-in (Update 2)", kind="image",
         shipped=U2 + "/Exporters/IMFLithDTX.dll", stem="IMFLithDTX",
         # src/Engine/tools/Plugins/Plugins.dsw (MayaDTXLoader). As in our
         # map: dtxmgr, genltstream, MayaDTXLoader, pixelformat, streamsim,
         # then Lib_StdLith's l_allocator.obj and dynarray.obj; from
         # 0x100063c0 the import thunks: MFC42.DLL's operator delete and new
         # (ordinals 825, 823), MSVCRT's _purecall, then msvcrt.lib's crtdll.obj
         # (__CRT_INIT, 0x100063d6)
         built="src/Engine/built/Release/tools/Plugins/MayaDTXLoader/IMFLithDTX.dll",
         app=[(0x10001000, 0x100063c0)]),
    # The 3ds Max and Maya plug-ins need those packages' SDKs, version 3
    # and 4 (Build 69 carries only Max 7's and Maya 6.5's and 7.0's).
    # Provisional ranges: to the import thunks before msvcrt.lib's
    # crtdll.obj (or to crtdll.obj where no thunk run precedes it).
    dict(label="MaxDTXLoader30.bmi", what="3ds Max .dtx bitmap loader for 3ds Max 3 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MaxDTXLoader30.bmi", stem="MaxDTXLoader30", built=None, optional=True,
         app=[(0x10001000, 0x10006724)]),   # provisional: to the runtime
    dict(label="MaxDTXLoader40.bmi", what="3ds Max .dtx bitmap loader for 3ds Max 4 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MaxDTXLoader40.bmi", stem="MaxDTXLoader40", built=None, optional=True,
         app=[(0x10001000, 0x10006884)]),   # provisional: to the runtime
    dict(label="MaxModelExport30.dle", what="3ds Max model exporter for 3ds Max 3 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MaxModelExport30.dle", stem="MaxModelExport30", built=None, optional=True,
         app=[(0x105b1000, 0x105bbc5f)]),   # provisional: to the runtime
    dict(label="MaxModelExport40.dle", what="3ds Max model exporter for 3ds Max 4 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MaxModelExport40.dle", stem="MaxModelExport40", built=None, optional=True,
         app=[(0x105b1000, 0x105bcaaf)]),   # provisional: to the runtime
    dict(label="MaxWorldExport30.dle", what="3ds Max world exporter for 3ds Max 3 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MaxWorldExport30.dle", stem="MaxWorldExport30", built=None, optional=True,
         app=[(0x10001000, 0x10012a52)]),   # provisional: to the runtime
    dict(label="MaxWorldExport40.dle", what="3ds Max world exporter for 3ds Max 4 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MaxWorldExport40.dle", stem="MaxWorldExport40", built=None, optional=True,
         app=[(0x10001000, 0x10012c52)]),   # provisional: to the runtime
    dict(label="MaxWorldImport30.dli", what="3ds Max world importer for 3ds Max 3 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MaxWorldImport30.dli", stem="MaxWorldImport30", built=None, optional=True,
         app=[(0x10001000, 0x1000d3f8)]),   # provisional: to the runtime
    dict(label="MaxWorldImport40.dli", what="3ds Max world importer for 3ds Max 4 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MaxWorldImport40.dli", stem="MaxWorldImport40", built=None, optional=True,
         app=[(0x10001000, 0x1000d4c8)]),   # provisional: to the runtime
    dict(label="MayaModelExport30.mll", what="Maya model exporter for Maya 3 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MayaModelExport30.mll", stem="MayaModelExport30", built=None, optional=True,
         app=[(0x10001000, 0x1000e16c)]),   # provisional: to the runtime
    dict(label="MayaModelExport40.mll", what="Maya model exporter for Maya 4 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MayaModelExport40.mll", stem="MayaModelExport40", built=None, optional=True,
         app=[(0x10001000, 0x1000e16c)]),   # provisional: to the runtime
    dict(label="MayaWorldExport30.mll", what="Maya world exporter for Maya 3 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MayaWorldExport30.mll", stem="MayaWorldExport30", built=None, optional=True,
         app=[(0x10001000, 0x1001ed78)]),   # provisional: to the runtime
    dict(label="MayaWorldExport40.mll", what="Maya world exporter for Maya 4 (Update 2)", kind="image",
         shipped=U2 + "/Exporters/MayaWorldExport40.mll", stem="MayaWorldExport40", built=None, optional=True,
         app=[(0x10001000, 0x1001ed78)]),   # provisional: to the runtime
]
