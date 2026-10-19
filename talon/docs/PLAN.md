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

No source; their descendant is Build 69's PreProcessor (Processor.exe of
the SDK build 51 is the Jupiter one, already a different program: it
writes world version 85).

- [ ] **Processor.exe (AvP2)**, cl 12.00.8966, static MFC, 340 KB of code.
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
