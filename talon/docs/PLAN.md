# The plan

Seventeen shipped binaries, about 2.26 MB of their own code
(`progress/targets.py`). The order is by what each teaches the next and by
what the evidence allows.

## 1. The plug-ins (Update 2, cl 12.00.8966)

Near-identical to the LithTech Jupiter SDK build 51's plug-ins (same code
sizes; the DTX loaders and model exporters differ in a handful of bytes),
whose descendants' source is Build 69's. Start from that source, compiled
with VC6 SP5, and bring it back.

- [x] **IMFLithDTX.dll** (Maya DTX image plug-in): done. Build 69's source,
  build 51's libraries and SDK headers, no TdGuard, StdLith and MFC's
  import library linked: 105 functions identical, 81 small, none left.
- [ ] **MaxDTXLoader30/40**, **MaxModelExport30/40**,
  **MaxWorldExport30/40**, **MaxWorldImport30/40**: need the 3ds Max 3 and
  4 SDK headers (class layouts changed between Max versions; Build 69 has
  only Max 7's). To find: the SDKs from the 3ds Max 3.x/4.x CDs.
- [ ] **MayaModelExport30/40**, **MayaWorldExport30/40**: need the Maya 3.0
  and 4.0 devkit headers and libraries (Build 69 has 6.5's and 7.0's).

## 2. The world processors

No source of any version; their descendant is Build 69's PreProcessor
(the SDK build 51's Processor.exe is already the Jupiter one, writing world
version 85). `progress/Processor-avp2-map.md` is the map.

- [ ] **Processor.exe (AvP2)**, cl 12.00.8966, static MFC, 340 KB of code.
  - [x] Its libraries: StdLith, zlib 1.0.4 and RegMgr from
    `src/LT2/lithshared` compile identically to the AvP2 release's prebuilt
    libraries (153/153, 82/82, 14/14): /MT, no /Ob2 for StdLith and zlib.
  - [x] The project (`src/LT2/tools/PreProcessor/Processor.dsw`), with the
    shared engine and world code taken from Build 69 (about 130 of its
    functions match as Build 69 has them). Not in the default build until
    it compiles: `make -f build/build.mk WITH_PROCESSOR=1`.
  - [ ] Bring the shared classes back to 2001. Their class layouts come from
    the shipped vtables (tools/guides/07, `vtabaudit.py`, `vtabmap.py`): the
    LTA load/save methods Build 69 has are not in the 2001 classes, and
    removing a virtual changes every call through the vtable, so each class
    is laid out from the image, not trimmed by hand. Compiled against build
    51's SDK headers (src/Engine/sdk/inc, the tool code's TVector3) with the
    LT2 libraries.
  - [ ] The core, module by module in image order (the map). Its objects
    are scored before the image links (`partial_objs` in
    progress/targets.py).
    - [x] BasePoly, bindmgr (0x401000..0x402a70): matched.
    - [~] BrushToWorld (0x402a70..0x404bf0): the 2001 CSG version, written
      from the image. 62 of its 69 compiled functions identical; five near
      (inline budget, guide 09).
    - [~] BspGen (0x404c10..0x407e80): Build 69's brought back to 2001.
      120 of its 125 compiled functions identical; four near
      (GenerateBspTree, AddReversedPoly, SplitPoly,
      MinimizeNodePolySize).
    - [ ] The next modules (0x407e80..), problem brushes and leak files (0x40b540..), DTX output,
    world properties and texture flags (0x411690..), lighting (0x415cb0..),
    physics and terrain (0x425d20..), the MFC application and its dialogs,
    the main processing driver and DAT output (0x42f900.., 0x43e7e0..).
- [ ] **Processor.exe (NOLF)**, cl 12.00.8447, 415 KB: 85% the same
  strings as AvP2's; after AvP2's, as its diff.

## 3. The editors (cl 12.00.9044, static MFC)

- [ ] **ModelEdit.exe** (221 KB of its own code), then **DEdit.exe** (573
  KB). Their RTTI names their classes (135 and 101 type descriptors); the
  shared libraries inside them (ButeMgr, StdLith, RegMgr, DibMgr: DEdit
  0x4835a0..0x48ba82) match the AvP2 source release's prebuilt libraries
  in places, and build 51's (src/Libs) are their near relatives.

## Method

As tools/guides/ describes: the shipped function's bytes (`exedump.py`),
our source, compiled one object at a time (`make -f build/build.mk <obj>`
in `make shell`), compared (`fndiff.py`, `exematch.py`), recorded
(`make progress-update`). Ghidra (`tools/ghidra/decomp.sh`) is a reading
aid, never source.
