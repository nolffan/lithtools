# Bugs in the retail tools

Bugs in the Talon-era tools as they shipped (AvP2 v1.0's and Tools
Update 2's, NOLF Tools 001's), found while rebuilding them. Each is a
mistake in the code that a source change would fix. This is a living list:
add a bug when it is found, in the format below, whether or not it is
fixed yet.

Our sources reproduce every one of them, on purpose: compiling to the
retail machine code means compiling its mistakes too. The tools are to be
re-implemented later with these fixes applied (unlike the games, which fix
theirs in a modern build of the same sources); this list is what that work
starts from. An entry that is fixed then says so under its heading, with
where the fix is.

Every binary here is a reconstruction (`docs/PLAN.md`), so an entry is
read off the retail bytes at the address it names, never guessed, and says
how well our function matches the retail one there (`tools/exematch.py`,
`tools/fndiff.py`): identical, or near with the bug reproduced. Addresses
are as in the retail images (`build/reference.sh` extracts them into
`out/reference/`).

## Format

```
### <What goes wrong, as a level designer or a programmer would put it>

<binary> <address> (<function>, <source file>; identical or near). What the
code does, and what it should do; how it shows, if it does.

Not fixed yet.
```

## The bugs

### The Processor's ray selection count covers only the last world model

AvP2's Processor.exe 0x4450c0 (DoRaySelect,
src/LT2/tools/PreProcessor/RaySelecter.cpp; near: size-exact, one
multiply's operand order differs). Each RaySelecter object casts a ray at
every world model's polies and selects the brush of the last poly it hits.
The count of selections is cleared at the start of each world model
(0x4450e7) instead of once before the loop, so the function returns how
many rays selected something in the last world model only. Every selection
is still made; only the count is wrong, and it goes no further than the
log's "RaySelect: Did %d ray selections" (Processing, 0x440400).

Not fixed yet.

### Merging terrain surfaces skips a poly after each one it takes out

AvP2's Processor.exe 0x445b80 (MergeSurfaces_RS, the name ours,
src/LT2/tools/PreProcessor/RaySelecter.cpp; near: the same instructions,
one register spilled). MakeTerrain calls it (0x446714) to give polies with
the same surface one surface: the polies are hashed into 512 lists, and
each list's poly at i is taken out (0x445c67) and compared with the polies
left. Taking poly i out moves the next one into slot i, and the loop then
goes on to i + 1, so that poly is never compared as the first of a group.
Polies with the same surface can keep surfaces of their own: the terrain
keeps more surfaces than it needs. Nothing breaks.

Not fixed yet.

### A DTX texture with 9 to 15 mipmaps overflows the heap

AvP2's Processor.exe 0x40d380 (dtx_Create, the 2001 dtxmgr,
src/LT2/tools/PreProcessor/EditBrush2001.cpp; identical). The header check
accepts up to 15 mipmaps, but TextureData (0x18c bytes) has only 8 mip
records, so a texture claiming 9 to 15 makes dtx_Alloc and the mip reads
write past the end of the block. Build 69's dtxmgr and the engine's copy
have the same check. Only a malformed texture shows it.

Not fixed yet.

### Loading a brush with more than 65535 points or polies never ends

AvP2's Processor.exe 0x40dae3 and 0x40dbcc (CEditBrush::LoadTBW,
EditBrush2001.cpp; identical). The point and poly counters are WORDs, so
for a count over 65535 the loops wrap round, never finish and write past
the arrays. Only a malformed or very large .ed file shows it.

Not fixed yet.

### A brush that fails to load leaves garbage pointers to be deleted

AvP2's Processor.exe 0x40da20 (CEditBrush::LoadTBW, EditBrush2001.cpp;
identical). When a poly fails to load, the m_Polies slots after it are
left uninitialised (BaseNew<CEditPoly*>, 0x40f600, doesn't clear them),
and the caller (CEditRegion::LoadFile, 0x410870) has already put the brush
in the region. Term then deletes every non-null slot, garbage included. A
corrupt or truncated .ed file shows it.

Not fixed yet.

### A failed .ed load leaks the brushes loaded so far

AvP2's Processor.exe 0x410570 (CEditRegion::LoadFile,
src/LT2/tools/PreProcessor/EditRegion2001.cpp; identical). Brushes go into
m_Brushes and the brush indexer before the node tree links them, but the
region deletes brushes only through the tree (Term's
m_Brushes.RemoveAll() frees only the list's links). When a brush fails to
load (0x410879) it and every brush before it leak; when the node tree
fails or an exception is caught, every brush not yet in the tree does;
and the indexer stays allocated until the next load. Nothing shows: the
Processor stops after a failed load.

Not fixed yet.

### A texture name longer than 100 characters is lost

AvP2's Processor.exe 0x40fc08 (LoadEditPolyTBW,
src/LT2/tools/PreProcessor/EditObjects2001.cpp; identical). A poly's
texture name over 100 characters is replaced with an empty string instead
of being rejected or kept, so the poly silently loses its texture.

Not fixed yet.

### Making the visibility BSP carries on after it fails

AvP2's Processor.exe 0x40ce37 (MakeVisBSP, the name ours,
src/LT2/tools/PreProcessor/MakeBsps.cpp; identical). The result of the
second MakeBsp (after removing unseen geometry) is ignored, so a failed
visibility BSP goes on into the PVS; the physics BSP's path checks the
same call. 0x40cec7 also ignores SetupVisBSPLeaves' result (MakeWorldPVS
calls it again and checks).

Not fixed yet.

### Filtering a poly into the leaves leaks the pieces of a failed split

AvP2's Processor.exe 0x40cb61 (FilterPolyIntoLeaves_R, the name ours,
MakeBsps.cpp; near: one inlined dot product). When SplitPoly gives up, the
function returns without freeing the two pieces SplitPoly already made.
A memory leak during processing; nothing shows.

Not fixed yet.

### Splitting a poly list keeps polies SplitPoly gave up on

AvP2's Processor.exe 0x40c5fb (SplitPolyList, the name ours, MakeBsps.cpp;
identical). SplitPoly's result is ignored, so a poly it gave up on (more
than two edge splits) still goes on, in whatever pieces it left.

Not fixed yet.

### Terrain blockers free malloc'd pieces with delete

AvP2's Processor.exe 0x40c209 (AddTerrainBlockers, the name ours,
MakeBsps.cpp; near: the inline budget at two list destructors). The split
pieces were allocated with malloc and are freed with scalar delete; it
works only because the runtime's operator delete calls free.

Not fixed yet.

### Terrain occluder polies are unlinked but never freed

AvP2's Processor.exe 0x40bad0 (RemoveTerrainOccluderPolies, the name
ours, MakeBsps.cpp; identical). The polies are taken out of the world's
list and left allocated: a leak during processing; nothing shows.

Not fixed yet.

### The vis leak file name is appended with the whole buffer's size

AvP2's Processor.exe 0x40cd6b (MakeVisBSP, MakeBsps.cpp; identical).
`strncat(buf, "_Vis.lek", 270)` passes the size of the buffer rather than
the room left in it. Latent: the 255-character limit on the leak file
name keeps it inside the buffer.

Not fixed yet.

### Yes/no questions return MessageBox's code unchanged

AvP2's Processor.exe 0x430510 (AskQuestion,
src/LT2/tools/PreProcessor/ProcessorApp.cpp; identical). There is no
return after the IDYES, IDNO and IDCANCEL checks, so any other result
falls out with MessageBox's own code. Latent: with MB_YESNO and
MB_YESNOCANCEL nothing else comes back. (VC6 warns, C4715.)

Not fixed yet.

### The status window can stop updating once it fills

AvP2's Processor.exe 0x430220 (DrawStatusTextToScreen, ProcessorApp.cpp;
identical). Each line is put at the caret after selecting from
GetLimitText() instead of the text's length, so once the edit control
reaches its text limit, new status text may no longer show (read from the
window messages, not run). The log file is unaffected.

Not fixed yet.

### Two NULL checks are missing in the application

AvP2's Processor.exe 0x42fdb1 (CPreProcessorDlg::OnInitDialog) and
0x430640 (CPreProcessorThread::InitInstance), ProcessorApp.cpp; identical.
The AppWizard's NULL check on GetSystemMenu was removed, and the thread
calls g_pAppDlg->GetDlgItem without checking g_pAppDlg (its other branch
does). Latent: both are always there when these run.

Not fixed yet.

### Saving the DAT file can go on with an unset result after "No"

AvP2's Processor.exe 0x44170d (SaveDATFile,
src/LT2/tools/PreProcessor/Processing.cpp; identical). Answering No to
"Error opening output file. Try again?" leaves the result variable
unset, so the function returns whatever was on the stack; when that is
nonzero, it goes on to remove the existing DAT file and rename a temp
file that was never created.

Not fixed yet.

### SaveDATFile's rename error messages are garbled

AvP2's Processor.exe 0x4415a0 and 0x4417c5 (SaveDATFile, Processing.cpp;
identical). The message buffer is filled only when the remove fails, so a
failed rename after a successful remove shows stack garbage at the start
of its message; and the message is formatted into itself, so each retry
prepends the previous one.

Not fixed yet.

### Surfaces made invisible by invisible.dtx still get lightmaps

AvP2's Processor.exe 0x43fa90 (GenerateSurfaces, Processing.cpp;
identical). Surfaces forced invisible because their texture is
textures\invisible.dtx keep SURF_LIGHTMAP, so lightmaps are made for
surfaces nobody sees (Build 69 clears the flag): wasted lightmap space in
the DAT.

Not fixed yet.

### The AI-volumes path takes the maximum lightmap size from the grid size

AvP2's Processor.exe 0x440e80 (DoProcessing, Processing.cpp; identical).
On the -aivolumes path, m_MaxLMSize is set from the DAT file's lightmap
grid size rather than its maximum lightmap size. Read from the code, not
confirmed by a run.

Not fixed yet.

### Light animations with too many frames, polies or vertices corrupt the DAT

AvP2's Processor.exe 0x432a40 (CPreMainWorld::SaveLightAnims,
src/LT2/tools/PreProcessor/PreWorld.cpp; near: two scheduling
differences). Each animation's frame count is written as a byte, its poly
count as a WORD and each poly's vertex count as a byte, while the data
behind them is written in full. Over 255 frames or 65535 polies makes the
file unreadable for LoadLightAnims; over 255 vertices loses colours.
Latent with normal levels.

Not fixed yet.

### The lightmap compression ratio divides by zero without light animations

AvP2's Processor.exe 0x433520 (CPreMainWorld::CalcLMCompressionRatio,
PreWorld.cpp; near, size-exact). The compressed size isn't checked for
zero, so a world with no light-animation data gets 0/0 (NaN) as
m_LMCompressionRatio in the statistics it prints.

Not fixed yet.

### Texture lights leak on failure, and divide by zero on a perpendicular plane

AvP2's Processor.exe 0x419e8d and 0x41a60b (CLightMapMaker::
GetTextureLights, src/LT2/tools/PreProcessor/LightMapMaker_r3.cpp; near).
When GetTextureInfo fails, the function returns without freeing what it
already put in its texture info list (a leak, only on the "Unable to get
texture lights" path). And if a texture's plane is perpendicular to its
poly, the tiling divides by zero (N . vNormal); the point then fails
InsideConvex, so no light is made for that texture there.

Not fixed yet.

### Moving a lightmap sample out of a wall never tests one corner

AvP2's Processor.exe 0x41ca1a..0x41ca9f (CLightMapMaker::
CalcSamplePositions, LightMapMaker_r3.cpp; near). The ring search for a
clear supersample stops both row searches before x = gx + r and both
column searches before y = gy + r, so the corner (gx + r, gy + r) is never
tried: a sample in a wall can move to a slightly farther clear point than
the nearest.

Not fixed yet.

### Tracing a light ray has a dead test and no stack bound

AvP2's Processor.exe 0x4181f8 (TraceSegment,
src/LT2/tools/PreProcessor/LightMapMaker_r2.cpp; near: an unused frame
slot). It compares the low WORD of the split node's CPrePoly* with
0xFFFF, a leftover of a WORD poly index: a pointer never has that low
word, so the test is always true (a node with no poly would crash). And
it pushes onto its 1024-entry stack without a bound, so a BSP deeper than
1024 splits along one ray would write past it. Both latent.

Not fixed yet.

### Unclipped lights give shadow-map animations colour instead of a mask

AvP2's Processor.exe 0x417180 (CLightMapMaker::ApplyLightToPoly,
LightMapMaker_r2.cpp; near). With clipping off, globally or for the
light, shadow-map animations get the light's normal colour from
CalcLightSample instead of the 255/0 mask, and the directional cone test
is skipped, so those shadow maps show graded colour where every other
light gives a full on/off mask. Read from the code, not confirmed by a
run.

Not fixed yet.

### Labelled world nodes leak their labels

AvP2's Processor.exe 0x44e750 (CWorldNode::Term,
src/LT2/tools/shared/world/WorldNode.cpp; identical) and 0x44e5c0 (its
destructor, which only calls Term). The 2001 Term never frees the node's
label, so every labelled node leaks its label string.

Not fixed yet.

### Opening a stream leaks the file handle when the stream can't be made

AvP2's Processor.exe 0x445910 (streamsim_Open,
src/LT2/tools/shared/engine/streamsim.cpp; identical). If `new SSFile`
fails, it returns NULL without closing the file it has just opened.

Not fixed yet.

### Console commands past the 30th argument are dropped

AvP2's Processor.exe 0x40a230 (cp_Parse,
src/LT2/tools/shared/engine/conparse.cpp; identical; Build 69 does the
same). At 30 arguments it stops and reports no more commands, silently
dropping everything after the 30th token, including any later
;-separated commands.

Not fixed yet.

### The physics BSP's T-junction splits keep uninitialised vertex colours

AvP2's Processor.exe 0x4475a0 (sub_4475a0, the T-junction split,
src/LT2/tools/PreProcessor/Terrain.cpp; identical). MakeBsp (0x43eea0)
sets every vertex colour of the BSP's polies to 255 (lighting does not
need them), and only then does MakeWorldModelBSP fix T-junctions
(0x43f810, FixTJunctions 0x4479c0). With -FixTJunc (on by default) a poly
split there gets two new polies from CreatePoly (malloc, nothing cleared)
that AddVert gives positions and normals but no colours, and the .dat
saves the colours as they are (CPreWorld::SaveFile, 0x436ad0, truncated
to bytes). So the physics BSP's split polies carry whatever the heap held:
the shipped exe's own output changes with nothing but the length of the
exe's name (Sample_DM.dat: 848 polies' colours), and a run slowed down
(by logging every allocation) differs the same way, so the timing of the
dialog thread's allocations counts too. Harmless as far as is known (the physics BSP
is not drawn), but it makes the .dat depend on the run.

Not fixed yet.

### The Max model exporter hangs on a skinned node whose first modifier isn't Physique or Skin

MaxModelExport30/40.dle 0x105b27d0 (ExportSkeletalPiece) and 0x105b3d70 /
0x105b4320 (ExportSkeletalBindings, 30 / 40), both in
src/Engine/tools/Plugins/MaxModelExport/Exporter.cpp; identical. Each
walks the node's derived-object chain with `while (obj &&
obj->SuperClassID() == GEN_DERIVOB_CLASS_ID)` looking for the Physique or
Skin modifier, but never advances `obj` inside the loop: if the first
derived object holds neither, the export loops forever. DeterminePieceType
does walk the chain (`GetObjRef()`), so a modifier further down marks the
piece skeletal and leads straight into the hang.

Not fixed yet.

### The Max model exporter writes a byte past the base texture path's buffer

MaxModelExport30/40.dle 0x105b4d50 / 0x105b5ad0 (GetTextureInfo,
src/Engine/tools/Plugins/MaxModelExport/Exporter.cpp; identical).
`new char[strlen(path)]` then `strcpy` into it: one byte short for the
terminator, a heap overrun on every texture path.

Not fixed yet.

### The Max model exporter samples vertex animations at the wrong times

MaxModelExport30/40.dle 0x105b1cd0 (ExportVertexAnimations,
src/Engine/tools/Plugins/MaxModelExport/Exporter.cpp; identical). The
start is computed in frames (`Start()/GetTicksPerFrame() + 1`) and then
used as ticks (`curTime = start + i*GetTicksPerFrame() + 1`), so unless
the animation range starts at 0 the samples are taken from the wrong
times.

Not fixed yet.

### The Max model exporter deletes a TriObject with delete instead of DeleteThis

MaxModelExport30/40.dle 0x105b1f50 (ExportVertexAnimation,
src/Engine/tools/Plugins/MaxModelExport/Exporter.cpp; identical). The
TriObject from ConvertToType is freed with `delete tri`, unlike every
other export path's `DeleteThis()`: across the plug-in/Max heap boundary
that can corrupt Max's heap.

Not fixed yet.
