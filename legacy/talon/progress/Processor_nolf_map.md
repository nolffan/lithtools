# Processor.exe (NOLF Tools 001): the map of the image

Built 2001-01-03 (writes NOLF's world version 66: the DAT version is
checked at 0x4433b6 and written at 0x4437d3, AvP2's 70). 2,723 rows of the
function table in Monolith's code, 0x00401000..0x00469000.

## What built it

The Rich header: 67 C++ and 10 C objects of cl 12.00.8447 (VC6 SP3), the
C runtime's (159 C and 11 C++ objects of 8168, 40 of masm 7299, 2 AliasObj),
cvtres 1735, and 82 objects without `@comp.id`.

- **The processor's own code is Intel C/C++ 4.0** (`toolchains/icl-4.0`),
  `/O1`, not Visual C++. The 82 objects without `@comp.id` are icl's (VC5's
  have none either, but VC5 does not make this code, nor do 8447's /O1 or
  /O2): the EH frame with `sub $N,%esp; push %edi; push %edi; push %esi; push
  %ebx` and the stack-homed `this` (0x401000), `lea 0x0(%esi),%esi` padding
  (0x460ec1), every function 16-byte aligned, and the literals in a
  `.data1` section (0x495000..0x49c890), icl's. A test class compiled with
  icl 4.0 `/O1` (or `/Os`, or `/O2 /G6 /Os`) gives 0x401000's
  instructions exactly; 4.5 and 5.0.1 do not (`mov $4,%ecx; add %eax,%ecx`
  where 4.0 and the image have `lea 0x4(%eax),%ecx`). AvP2's 54 sources
  compiled with icl 4.0 match 1,966 functions of the image uniquely at
  `/O1`, 1,819 at `/Os`, 253 at `/O2 /G5`, 31 at `/O2`; 4.5 `/O1` 104. On a
  subset, `/G6`, `/Gy`, `/Ob1`, `/Oi-`, `/Qrcd` change nothing, `/G5`,
  `/Ob0`, `/Ob2`, `/Qip`, `/Oy-` lose. None of Intel's libraries is linked
  (libscan finds no libm, libmmt, libirc member); the link needs their
  directory only because icl's objects name libmmt.lib as a default library.
  Our link of the same sources has 71 objects without `@comp.id`: NOLF's
  processor had about 11 more source files than AvP2's 54.
- **PreWorld.cpp is VC6 SP3's** (cl 8447) `/O1`: 0x442f08..0x44ce10 is
  unaligned, `__EH_prolog` (0x46b5b8) code with `and $0`, push/pop
  constants, the /O1 shapes. AvP2's PreWorld.cpp compiled so matches 196
  functions there; the one object besides MFC, StdLith and RegMgr that
  makes the C++ 8447 count 67 (ours is 67 too).
- **StdLith, zlib 1.0.4 and RegMgr are VC6's** (8447 or 8168: the two give
  the same code; the Rich header says 8447): 8447 matches 118 of their
  functions anywhere in the image, VC5 70; zlib's 10 C objects are the
  Rich header's C 8447 row. The AvP2 release's sources, as
  `src/LT2/lithshared` has them, with the NOLF_* projects
  (`Lib_StdLith_NOLF.dsp` and the others, build.mk).
- **MFC is msvc-6.00-8447's nafxcw.lib** (8447: its dlgcore.obj's
  CreateDlgIndirect, 0x47b492, is in the image and not 8168's; its
  thrdcore.obj's AfxTermThread, 0x47ac56, is not 8966's), the C runtime
  that tree's libcmt.lib; linker 6.00.
- **No STLport strings**: none of AvP2's `_STL` exception names or
  `basic_string`; the image has `std::bad_alloc`'s type descriptor and "bad
  allocation", thrown by an `operator new` loop at 0x44b69a (in PreWorld's
  VC object, called twice). Which STL (if any) the icl objects use is
  open: built against MS's STL instead of STLport ours matches 8 fewer.
- The C runtime members ours links and the image doesn't: rename, unlink,
  rmdir (and their w* forms), snprintf, strncat, 87triga (AvP2's SaveFile
  .tmp/rename logic, `_snprintf`, an inverse trig function). The image's
  that ours lacks: rtti.obj (a `dynamic_cast` or `typeid` somewhere).

## How icl 4.0 compiled it (what the first round learned)

- **/EHa.** A new-expression keeps `this` in a stack slot it never reads
  and stores the result twice more (CBoolProp::CreateSameKind 0x454b80,
  streamsim_Open 0x455a00): icl 4.0 gives that with /EHa, not /GX alone.
  Locals and `this` live in stack slots, not registers, under it.
- **Nothing is inlined into a function with an EH frame, nor is a
  function that has one inlined anywhere.** Elsewhere icl inlines as
  usual. So the small helpers the image has out of line (CMoArray's
  constructor, Init, GetSize, operator[]; ILTStream's 13-byte implicit
  constructor 0x456ab0; MFC's AfxGetApp, IsIconic, ...) are there because
  EH-framed functions call them. Our icl 4.0 (build 99100, the trial)
  inlines into those too, under every switch and pragma tried (/EHs,
  /Ob1, /Qip_no_inlining, /Qvc6, `#pragma inline_depth(0)`,
  `auto_inline`, mcpcom without `-mGLOB_new_eh`). The other Intel
  compilers are further off: 4.5 keeps few of 4.0's matches, and 5.0.1
  (build 010525Z) matches almost nothing (streamsim, PropList,
  LightMapMaker, WorldNode and EditBrush: 6, 1, 1, 1 and 1 functions,
  against 4.0's 52, 72, 309, 105 and 60) and inlines into EH-framed
  functions too. So it is 4.0, and probably another build of it than
  ours (the original 5.0, unheld, is unlikely if 5.0.1 is this far).
  Nor is it a back-end setting of ours (mcpcom's `-m` options, which icl
  passes through). `/Ob0` is `-mIPOPT_inline_limit=0
  -mIPOPT_opt_mask=0xfffe`; clearing bit 4 of the mask alone does the
  same (bit 7 changes other code; the rest change nothing here). The
  inliner's reasons for not inlining (its trace strings: size limits, call
  weights, recursion, asm, `#pragma optimize` off, a dtor thunk) have none
  about EH. `-mIPOPT_inline_max_total_stats` is the only threshold that
  moves anything; every value tried (150..400; the default lies above)
  matches more functions but fewer bytes over the icl objects. The inline
  script (`/Qip -mIPOPT_script_control=2 -mIPOPT_script_output=`) dumps
  every function's inline tree, but reading one back
  (`script_control=1`, `-mIPOPT_script_input=`) turns all inlining off in
  a one-file compile, so it can't steer call sites either. /Ob0 for a whole file is the
  stand-in where it wins (Processor.dsp: streamsim, ProcessorApp,
  EditObjects, create_world_tree, BuildHulls).
- **`X$$1`** is a second entry point: a function with 8-byte locals gets
  a 14-byte realigning stub (`push ebx; mov ebx,esp; and esp,-16; ...;
  jmp`) and its body, `$$1`, which aligned callers call directly. The
  function table splits them, as it does each function's unwind funclets.
  What makes icl align a frame isn't always visible (InitProcessOptions
  has no double on its stack).
- **`if(!x) return CONST;`** as its own statement gives a `jne +2; jmp
  <shared return>` island; folded into an enclosing if or `&&` it gives a
  plain `je`. A void function's early return copies the epilogue.
- **Blocks follow the source:** an if's first arm falls through; a
  ternary puts its second operand out of line.
- **Register choice follows statement order** where statements don't
  depend on each other, and moves with unrelated code in the object (the
  same inlined SetSize in CompressLightMap and CompressShadowMap). Unused
  locals change operand order the way VC6's declaration count does
  (BasePoly: Intersect or Left matches, by the count alone).
- A constant divisor reached through a local isn't reduced to a shift
  (ShowStatusText 0x44f1fe). Floating-point values live across a call are
  spilled as doubles. `T *p = new X;` takes one more slot than declaring
  `p` and then assigning it.
- Every function is its own COMDAT, so the unreferenced ones aren't in
  the image (not missing rows); a file's functions come out first, then
  the COMDAT copies of the inlines they called.
- The headers are NOLF's (sdkNOLF/inc, lithshared/stdlithNOLF, from
  nolfsrc): ILTMath without SetupEuler(vec) or GetEulerAngles,
  `sb_Free` testing only the object. PreWorld.cpp's STL is older than
  our STLport 4.0 (SGI 3.x: the deque's map first, 42-element buffers,
  the node allocator's `(n+7)>>3`): not yet a tree of ours.

## What the second round learned (2026-11-13)

Nine agents, one per group of modules, 1,344 -> 1,374 identical.

- **The inlining limit is not most of the gap.** Built three ways (as
  the project is, every icl file /Ob0, every one inlining) and the best
  taken per function, the image matches only 51 more functions (5 KB)
  than as built: the rest of the 196 KB of nears is ordinary source
  work. A file-wide /Ob0 was measured on every icl file this round; it
  wins only where Processor.dsp already has it (streamsim, ProcessorApp,
  EditObjects, create_world_tree, BuildHulls), and in each of those both
  ways lose recorded matches.
- **Its refinements:** an out-of-line copy of an inline (emitted because
  an EH-framed caller calls it) has nothing inlined into it either
  (GetPolySide 0x40b880, CalcExtents 0x45a520); calls to empty inlines
  are still dropped in EH-framed functions where our /Ob0 calls them
  (CRect() in OnPaint 0x441a30); icl inlines ordinary functions of the
  same file, not only `inline` ones (DrawStatusText into ShowStatusText
  0x44ef30), but never one defined later in the file than its caller.
  `#pragma inline_depth(0)` and `auto_inline(off)` don't stop it; only
  `#pragma optimize("", off)` does.
- **The whole object's register allocation moves with any edit in it:**
  CMoArray<CPrePolyRef>::Insert2 (0x42e660, emitted after LightMapMaker's
  own functions) matched or not as unrelated LightMapMaker edits came and
  went. Score the whole object after a change, not just the function.
- **Source shapes that show in the bytes:** if arms are laid out as
  written and icl doesn't normalise a negated condition (0x432847); sums
  are added in source order (0x4570fc); runs of `x = 0;` come out in
  reverse (0x42a1b1); stack slots follow declaration order (0x42a14d); a
  load through a pointer written twice is loaded twice (0x42a207);
  `VEC_DISTSQR` gives the z, y, x order `(v - w).MagSqr()` doesn't
  (0x42332e); a test through an inline helper changes block layout
  (0x45bbe0, 0x44f270); `if(...) ret = K; else if ...` on an int groups
  the compares (0x44284a); a recursive function's tail calls become a
  loop (0x422f45).
- **What no source form tried reaches:** where the frame's `add $N,%esp`
  falls among the last instructions (about a dozen GenFindElement copies,
  0x4027a0, 0x4546e0); a pointer the shipped code keeps in a register
  across a loop's back edge (0x42a401); some early returns placed after
  the function's `ret` (0x40ba3f, 0x40a755); floating-point operand order
  in places (quat_Slerp 0x42fe2e). They look like differences between
  our icl 4.0 build and Monolith's.
- **PreWorld (cl 8447):** `p[i]` gives a strength-reduced pointer where
  `*p++` doesn't (0x447960); a chained `file << r << g << b` keeps a slot
  per byte (0x446688); struct assignment uses `movs` only with esi and
  edi free, so member copies give 0x4451d7. NOLF's CPreLightFrame is 0x14
  bytes, m_PolyMaps alone (BaseNew 0x44ca8b, the destructor 0x44302f):
  ours still has AvP2's m_Unknown_14.
- **Open at the link:**
  - NOLF has a source file AvP2 doesn't, EditPlane.cpp (GetPolySide
    0x418540, SplitPoly 0x418690, the two-array SplitPoly 0x418e10, in
    EditObjects' range). It's written but not in Processor.dsp: in file
    order it matches GetPolySide, but the link then keeps PrePoly.obj's
    copy of the folded `_AllocateTArray<CEditVert>` and one more function
    is unplaced.
  - The image keeps functions nothing references (0x414a40, 0x414bf0,
    0x415130, 0x414670, 0x413610: no call, no address anywhere), which
    our link drops, so ours of them are "unlinked". Either the icl
    objects weren't all COMDATs or the link didn't /OPT:REF them.
  - `_MSC_VER` was above 1100 in the shipped build (probably icl's
    /Qvc6, ours is the default /Qvc5): CreatePoly's unwind funclet
    (0x405180) is the empty placement `operator delete`, which
    object_bank.h declares only then.
- **Build switches, measured on the whole target (1,374 identical as
  built):** `/Qvc6` everywhere keeps the count but loses 1,174 identical
  bytes and places 3 fewer; `/OPT:NOREF` gives 175 unwritten rows a
  counterpart (479 -> 304) but adds 158 functions of ours the image
  doesn't have (unplaced 57 -> 215, and 5 fewer identical); `/Gy-`
  (icl's ordinary functions in plain sections, inlines still COMDATs)
  loses 53 identical. So none is taken. But `/Gy-` is very likely the
  original's: icl's objects here put every function in a COMDAT
  (ordinary ones NODUPLICATES, inlines ANY), so /OPT:REF drops an
  unreferenced ordinary function, which the shipped image keeps (the
  EditBrush ones above) while dropping unreferenced inline copies, as
  plain sections plus COMDAT inlines would. It costs now because a call
  within one plain section has no relocation: its displacement only
  matches once every function before its target in that object has the
  shipped size. Take it when the objects' layouts are right.

## What the third round learned (2026-11-13)

Eight agents, 1,374 -> 1,443 identical, the function table corrected
(2,579 -> 2,217 rows).

- **PreWorld.cpp's STL is SGI 3.2's**, now ours in lithshared/stl_sgi
  (Visual C++ 6 config, the node allocator and its spin lock, list,
  deque, heap, priority_queue over a deque of 42-element nodes), each
  piece checked against 0x449c8c..0x44ce10: PreWorld 275 -> 330. destroy
  is three levels out of line (0x44b615); functions taking deque
  iterators by value are never inlined under cl 8447 (0x44b63e).
- **NOLF's CPreLightFrame** is m_PolyMaps alone, 0x14 bytes, with the
  implicit constructor (0x42e8c0).
- **NOLF's zlib was built without /FR**: browse info changes cl 8447's
  code (inflate_blocks 0x466bf0 reloads cached pointers in another order
  with it). StdLith's NOLF project has /FR too: untested.
- **Our icl also inlines functions that have an EH frame**, the other
  half of the inlining limit. The shipped CreatePoly (0x405180, 0x40cfb0)
  and BaseNew/_AllocateTArray (0x435c30) are EH-framed because the
  placement `operator delete` exists (l_allocator.h, object_bank.h
  declare it only when `_MSC_VER != 1100`), and every caller calls them
  out of line. Under our /Qvc5 they have no frame and inline everywhere;
  declaring the delete gives them the frame, but ours then inlines them
  with it (FilterPolyIntoBSP 2,872 -> 5,082 bytes).
- **The shipped compiler sizes a frame as if it had inlined** where it
  didn't (MakeLeakFile 0x420bee: 0xac against our 0x80, with $$1
  alignment), so /Ob0 doesn't reach those either.
- **Out-of-line copies by definition order.** icl doesn't inline a
  function defined later in the file than its caller, so defining a
  helper after the EH-framed functions that call it, and before the ones
  that inline it, gives the shipped mix (DrawStatusText: called from
  SaveEDFile, inlined in ShowStatusText 0x44ef30). Ruling: a faithful
  shape where the helper's shipped copy sits where that definition (or an
  inline's out-of-line copy) puts it; where it would move a function off
  its shipped place, it's a stand-in and says so in its `talon:` comment.
  It doesn't hold for every function (~CFullLightMap is inlined into
  Term 0x424160 wherever it's defined).
- **Per-object lottery, more of it:** a header member removed in
  PreWorld.h swaps floating-point operands in PrePoly.obj (AdjustForVec
  0x43f2b0); every term order of MatVMul_H's third row gives the same
  bytes (x, z, y shipped, y, z, x ours, 0x4296cf); an edit in one object
  can change which copy of a folded Insert2 the link keeps (0x4126e0 vs
  0x41c2b0).
- **Dead code in the image.** A call graph of the shipped image (calls,
  and pointers in .rdata/.data/.rsrc) shows about 9 KB of EditBrush
  (MakeCap 0x412770 .. 0x4154f0), EditObjects 0x416fc0..0x418230,
  EditPlane's three functions, EditPoly 0x41a340..0x41a610 and EditRegion
  0x41bed0, 0x41c050 reached only from functions nothing references: kept
  by the plain-section build (`/Gy-`, above), dropped by ours, so ours of
  them are "unlinked" and can't score under the current link however
  right they are.
- **Smaller:** cl 8447 holds a return value in esi past a local's
  destructor and duplicates the destructor when esi is busy
  (LoadLightAnims 0x443af8); integer stores are scheduled on their own
  (TraceSegment 0x422e47); stack slots follow first use as well as
  declaration (0x41f590); a `switch`'s default arm goes after the unwind
  funclets (0x44dcd0); out-of-class members defined earlier in the file
  aren't inlined, unlike free functions (PortalBuffer::Cur).

## The EH split (2026-11-13): a stand-in

Monolith's icl 4.0 behaved as if every EH-framed function were compiled
/Ob0 and everything else not, inside one file; ours can only do one or
the other per file. So each icl object is compiled twice, as the project
says and with /Ob0 (into ob0/ beside it), and build/ehsplit.py rewrites
the object so that every function EH-framed in the /Ob0 compile (its
section has a relocation to `__except_list`) carries the /Ob0 code
(build.mk, `ehsplit`). The sources aren't touched; ob0/<stem>.ehsplit.txt
lists what each object took (134 functions in 53 objects at the start).

- Measured: 1,444 -> 1,599 identical, unwritten 123 -> 34. It replaced
  the whole-file /Ob0 stand-ins of BuildHulls, EditObjects, ProcessorApp
  and streamsim; create_world_tree keeps its /Ob0 on top (its /Ob0
  copies of CMoArray templates are the image's). Taking the /Ob0
  compile's out-of-line inline copies as well is much worse (1,455, 298
  unplaced): the image's copies are the inlining compile's in most
  objects.
- What it means for source work: an EH-framed function is now matched
  against code with nothing inlined into it, so its source is judged as
  the shipped compiler saw it. A near EH-framed function is a source
  difference now, not the limit, unless it inlines an EH-framed callee
  (CreatePoly and the allocation helpers, the other half, which the
  split doesn't reach: ours still inlines those into non-EH callers).
- What it isn't: Monolith's build. The image's code for these functions
  came from one compile; ours from two. It goes when a compiler build
  that does what Monolith's did turns up.

## What the fourth round learned (2026-11-13)

Six agents: five by address range over the near pairs, closest first,
and one on the unplaced functions. 1,443 -> 1,444 identical; unwritten
131 -> 123 and unplaced 69 -> 63 with six own_symbols pins
(progress/targets.py).

- **The per-function inlining ceiling, measured with the scorer:** built
  with every icl file /Ob0, 848 functions are identical, 193 of them
  (16.4 KB) not identical as built: as built or /Ob0, whichever matches,
  would be 1,636. (exematch's exact unique matches gave 51 and badly
  undercounted it.) LightMapMaker +39, Processing +30, BspGen +17,
  BasePoly +8, WorldApprox +8, the rest spread thin; mostly small
  functions and EH pieces. The shipped compiler behaved as if EH-framed
  functions were /Ob0 and the rest inlined, inside one file (BspGen: /Ob0
  gains 37, all EH-framed or their pieces, and loses 21, all plain). The
  way to take it is a stand-in: each file's EH-framed functions in a
  second source compiled /Ob0, /ORDER keeping the shipped placement.
- **Proven right, blocked only by the limit** (identical built /Ob0):
  MakeVisBSP 0x411ae0, FilterTerrainPolies_R 0x410d80, VisFlowPortal
  0x45b540, WorldTree() 0x45fa00, CWorldNode's constructor 0x460730 and
  destructor 0x4607e0, ProcessorOptions::OnOK 0x453a20, FindLeakPath
  0x420a40, FindWorldModels 0x41cf20, RecurseAndAddWorldModels 0x41cc70,
  PVSInfo() 0x451f50, GetWorldStats 0x44ed20; MakePhysicsBlocks 0x432a40
  agrees on all 102 calls.
- **CreatePoly** (EH-framed in the shipped build, the placement delete)
  is called out of line by every SplitPoly copy (0x403730, 0x407970,
  0x410600, 0x41118e, 0x43c280, 0x4589e0) and MakeBoxPortals (six times):
  none of them can match while ours inlines it.
- **What no source reaches, now ruled out by name:** GenFindElement's
  frame release (all 11 copies, 0x4027a0..0x450f40, release before the
  stores; six source forms and /G5 /G6 /Os /O2 /Op /Qpc32 /Qip /Gy
  don't move ours); MatVMul_H's term order, which differs between two
  inlines in one object (0x427a74, 0x423129); `this` moved to a stack
  slot at loop entry across LightMapMaker (0x42a3fe); repeated loads the
  shipped code merges or not where ours does the other (0x4295c3,
  0x42440e). icl 4.0 ignores `__declspec(noinline)` (warning #688);
  `#pragma optimize("g", off)` stops inlining but changes other code.
- **cl 8447 (PreWorld):** stack slots follow the set of variables and how
  they're used, not declaration order or names: one shipped slot where
  ours has two means one variable reused, an extra single-use slot an
  extra local. Matching LoadFile's slots this way (0x444b97, frame
  0x354) also fixed most of its registers (36 -> 7 differing blocks).
- **icl:** a `switch`'s default arm is the only form that goes after the
  unwind funclets (LoadDATFile 0x44de03, `switch(status == 1)`); trivial
  address-returning inlines are folded even in EH-framed functions
  (MakeZones 0x45d9db's `lea 0x20`); a function with no realign stub
  still gets a zero-length `X$$1` label; template members aren't bound
  by the EH rule (GenAppend 0x42c620, fully inlined).
- **Probably two source files we lack:** ProcessorApp's range holds a
  `PreProcessorThread.cpp` from 0x4424c0 and a `PreSurface.cpp` from
  0x442bd0 (the MFC inline copies' placement, the module order; Build 69
  has both names). Split out without /Ob0 they match five more and lose
  five recorded, to the limit.
- **Not landed, saved in out/drafts:** NOLF's SetupNewLightAnim puts each
  poly through ShouldLightmap (0x4257dd) and adds only lightmapped ones
  to the animation (ours, AvP2's, adds all: a behaviour difference), and
  GetLightDefs adds each light as soon as it's made (0x4265b3). Each
  flips Insert2 (0x42e660) or a placement; to re-try after the next
  change in LightMapMaker.obj.
- **Mispairings the near list makes** (by address; the scorer's, not the
  source's): 0x425aa0 + 0x425f04 are GetTextureLights; 0x427f70 +
  0x428814 CalcSamplePositions; 0x42b210 GenAppend<CMoArray<PVector>>;
  0x422d20 TraceSegment and 0x4230d0 LightWorldModelCB (swapped);
  0x42bc70 InternalNiceSetSize<CMoArray<CPrePoly*>>; 0x42f190
  _AllocateTArray<CMoArray<PVector>>; 0x403730 is ClipPoly$$1, 0x4042c0
  DoBrushesIntersect$$1, 0x406dc0 CalcSplitSelector, 0x408c60
  MinimizeNodePolySize; about 40 pairs in 0x40f840..0x4210f0 point at
  the dead code (third round).
- **What the scorer would need** (ext/tools is shared: described, not
  changed): place an icl realign stub only with its `$$1` body (all 26
  shipped stubs are byte-identical); ignore zero-length labels; pair by
  best rank across candidates, preferring the object's neighbourhood,
  not greedily in name order; let vtable slots and by-reference evidence
  override interchangeable copies (??_G, SetSize2, _AllocateTArray);
  take by-reference evidence from near callers' aligned call sequences,
  not only identical ones.

## Layout (libscan.py)

| Range | What | Source |
|---|---|---|
| 0x00401000..0x00460f00 | Processor's own objects (icl 4.0, but PreWorld VC6) | to reconstruct |
| 0x00460f00..0x00463390 | StdLith (l_allocator.obj's `_$E` initializers first, 0x460f00) | src/LT2/lithshared/stdlith, 8447 |
| 0x00463390..0x00468d90 | zlib 1.0.4 | src/LT2/lithshared/zlib, 8447 |
| 0x00468d90..0x00469000 | RegMgr | src/LT2/lithshared/regmgr, 8447 |
| 0x00469000..0x00469103 | MFC's inline functions (CMemoryException's and CNotSupportedException's constructors and destructors, AfxGetMainWnd 0x46909a, `??_9` vcall thunks 0x4690ae/0x4690b6, a destructor at 0x4690be/0x4690da) | the toolchain's |
| 0x00469103..0x004696ba | nafxcw.lib: wincore.obj's multimonitor stubs .. fixalloc.obj | the toolchain's |
| 0x004696dc..0x0047661a | libcmt.lib (exsup.obj .. setmode.obj) | the toolchain's |
| 0x00479de2..0x004821e7 | nafxcw.lib (except.obj .. appterm.obj) | the toolchain's |

## Modules

The order is AvP2's (the project's file order); the boundaries are
provisional, from the strings (below), the functions our objects match
uniquely and the pairings `tools/progress.py --nears` reports. Counts are
`make progress`'s statuses of the shipped rows in each range, for the first
link (2026-10-29: 1,058 identical).

| Range | Module | Source (src/LT2/...) | Rows | Identical | Near | Small | Todo |
|---|---|---|--:|--:|--:|--:|--:|
| 0x00401000..0x00402760 | BasePoly | tools/sharedNOLF/world/BasePoly.cpp | 32 | 10 | 8 | 4 | 10 |
| 0x00402760..0x00403300 | bdefs | tools/sharedNOLF/engine/bdefs.cpp | 27 | 22 | 2 | 0 | 3 |
| 0x00403300..0x004033f0 | bindmgr | tools/sharedNOLF/engine/bindmgr.cpp | 4 | 3 | 0 | 0 | 1 |
| 0x004033f0..0x00406860 | BrushToWorld | tools/PreProcessorNOLF/BrushToWorld.cpp | 90 | 33 | 18 | 9 | 31 |
| 0x00406860..0x0040b330 | BspGen | tools/PreProcessorNOLF/BspGen.cpp | 108 | 33 | 26 | 13 | 32 |
| 0x0040b330..0x0040db90 | BuildHulls | tools/PreProcessorNOLF/BuildHulls.cpp | 61 | 7 | 13 | 10 | 28 |
| 0x0040db90..0x0040dd60 | classbind | tools/sharedNOLF/engine/classbind.cpp | 4 | 3 | 1 | 0 | 0 |
| 0x0040dd60..0x0040e050 | conparse | tools/sharedNOLF/engine/conparse.cpp | 4 | 2 | 2 | 0 | 0 |
| 0x0040e050..0x0040fa60 | create_world_tree | tools/PreProcessorNOLF/create_world_tree.cpp | 28 | 13 | 7 | 2 | 6 |
| 0x0040fa60..0x00411dd0 | MakeBsps | tools/PreProcessorNOLF/MakeBsps.cpp | 49 | 11 | 16 | 8 | 14 |
| 0x00411dd0..0x004137b0 | dtxmgr | tools/sharedNOLF/engine/dtxmgr.cpp | 41 | 9 | 5 | 5 | 20 |
| 0x004137b0..0x00416c50 | EditBrush | tools/sharedNOLF/world/EditBrush.cpp | 103 | 23 | 20 | 10 | 42 |
| 0x00416c50..0x00419ca0 | EditObjects | tools/sharedNOLF/world/EditObjects.cpp | 93 | 12 | 17 | 11 | 51 |
| 0x00419ca0..0x0041b19a | EditPoly | tools/sharedNOLF/world/EditPoly.cpp | 48 | 10 | 10 | 4 | 23 |
| 0x0041b19a..0x0041cc70 | EditRegion | tools/sharedNOLF/world/EditRegion.cpp | 55 | 18 | 7 | 7 | 22 |
| 0x0041cc70..0x0041d780 | FindWorldModel | tools/PreProcessorNOLF/FindWorldModel.cpp | 19 | 6 | 5 | 2 | 6 |
| 0x0041d780..0x0041da50 | genericprop_setup | tools/sharedNOLF/engine/genericprop_setup.cpp | 8 | 7 | 1 | 0 | 0 |
| 0x0041da50..0x0041e1d0 | genltstream | tools/sharedNOLF/engine/genltstream.cpp | 6 | 6 | 0 | 0 | 1 |
| 0x0041e1d0..0x0041f1d0 | geomroutines | tools/sharedNOLF/engine/geomroutines.cpp | 11 | 9 | 2 | 0 | 0 |
| 0x0041f1d0..0x0041f540 | gettextureflags | tools/PreProcessorNOLF/gettextureflags.cpp | 17 | 8 | 3 | 2 | 3 |
| 0x0041f540..0x0041fcd0 | gettextureinfo | tools/PreProcessorNOLF/gettextureinfo.cpp | 15 | 1 | 2 | 3 | 9 |
| 0x0041fcd0..0x00420be0 | LeafPortals | tools/PreProcessorNOLF/LeafPortals.cpp | 8 | 1 | 3 | 1 | 3 |
| 0x00420be0..0x00421fb0 | LeakFile | tools/PreProcessorNOLF/LeakFile.cpp | 30 | 14 | 8 | 2 | 6 |
| 0x00421fb0..0x00422040 | light_table | tools/sharedNOLF/engine/light_table.cpp | 3 | 3 | 0 | 0 | 0 |
| 0x00422040..0x004223d0 | lightmap_compress | tools/sharedNOLF/engine/lightmap_compress.cpp | 7 | 3 | 4 | 0 | 0 |
| 0x004223d0..0x00422d20 | lightmap_planes | tools/PreProcessorNOLF/lightmap_planes.cpp | 7 | 4 | 2 | 0 | 0 |
| 0x00422d20..0x0042fb60 | LightMapMaker | tools/PreProcessorNOLF/LightMapMaker.cpp | 303 | 65 | 95 | 34 | 99 |
| 0x0042fb60..0x00430940 | ltquatbase | sdk/inc/ltquatbase.cpp | 6 | 3 | 4 | 0 | 0 |
| 0x00430940..0x00431080 | mathlt_impl | tools/sharedNOLF/engine/mathlt_impl.cpp | 14 | 9 | 5 | 0 | 0 |
| 0x00431080..0x00431b50 | Navigator | tools/DEdit/draw/Navigator.cpp | 23 | 9 | 5 | 4 | 4 |
| 0x00431b50..0x00431c90 | Node | tools/PreProcessorNOLF/Node.cpp | 4 | 3 | 0 | 0 | 1 |
| 0x00431c90..0x004325e0 | node_ops | tools/sharedNOLF/world/node_ops.cpp | 8 | 2 | 3 | 1 | 2 |
| 0x004325e0..0x00432a40 | parse_world_info | tools/sharedNOLF/engine/parse_world_info.cpp | 4 | 3 | 1 | 0 | 0 |
| 0x00432a40..0x00435dd0 | PhysicsBlocks | tools/PreProcessorNOLF/PhysicsBlocks.cpp | 66 | 30 | 15 | 3 | 18 |
| 0x00435dd0..0x0043c280 | pixelformat | tools/sharedNOLF/engine/pixelformat.cpp | 28 | 27 | 1 | 0 | 0 |
| 0x0043c280..0x0043dfb0 | PreBrush | tools/PreProcessorNOLF/PreBrush.cpp | 24 | 10 | 4 | 1 | 8 |
| 0x0043dfb0..0x0043e2c0 | pregeometry | tools/PreProcessorNOLF/pregeometry.cpp | 4 | 3 | 1 | 0 | 0 |
| 0x0043e2c0..0x0043ef50 | PreLeaf | tools/PreProcessorNOLF/PreLeaf.cpp | 39 | 11 | 5 | 5 | 18 |
| 0x0043ef50..0x0043f7e0 | PreLightMap | tools/PreProcessorNOLF/PreLightMap.cpp | 15 | 4 | 5 | 2 | 4 |
| 0x0043f7e0..0x00441240 | PrePoly | tools/PreProcessorNOLF/PrePoly.cpp | 24 | 8 | 11 | 1 | 4 |
| 0x00441240..0x00442f08 | ProcessorApp | tools/PreProcessorNOLF/ProcessorApp.cpp | 130 | 35 | 19 | 16 | 32 |
| 0x00442f08..0x0044ce10 | PreWorld | tools/PreProcessorNOLF/PreWorld.cpp | 366 | 206 | 96 | 24 | 39 |
| 0x0044ce10..0x00452b10 | Processing | tools/PreProcessorNOLF/Processing.cpp | 205 | 40 | 47 | 29 | 80 |
| 0x00452b10..0x00453fa0 | ProcessorOptions | tools/PreProcessorNOLF/ProcessorOptions.cpp | 39 | 11 | 9 | 4 | 11 |
| 0x00453fa0..0x00455050 | PropList | tools/sharedNOLF/world/PropList.cpp | 65 | 35 | 12 | 9 | 9 |
| 0x00455050..0x00455a00 | RaySelecter | tools/PreProcessorNOLF/RaySelecter.cpp | 15 | 6 | 3 | 1 | 5 |
| 0x00455a00..0x004574f0 | streamsim | tools/sharedNOLF/engine/streamsim.cpp | 89 | 29 | 21 | 14 | 18 |
| 0x004574f0..0x004589e0 | Terrain | tools/PreProcessorNOLF/Terrain.cpp | 25 | 4 | 15 | 1 | 5 |
| 0x004589e0..0x0045a7f0 | TJunctions | tools/PreProcessorNOLF/TJunctions.cpp | 15 | 2 | 5 | 1 | 7 |
| 0x0045a7f0..0x0045aad0 | Threads | tools/PreProcessorNOLF/Win/Threads.cpp | 13 | 13 | 0 | 0 | 0 |
| 0x0045aad0..0x0045d5c0 | VisLeaves | tools/PreProcessorNOLF/VisLeaves.cpp | 43 | 16 | 18 | 2 | 8 |
| 0x0045d5c0..0x0045e660 | WorldApprox | tools/PreProcessorNOLF/WorldApprox.cpp | 44 | 8 | 8 | 5 | 23 |
| 0x0045e660..0x004603e0 | world_tree | tools/sharedNOLF/engine/world_tree.cpp | 47 | 32 | 7 | 1 | 6 |
| 0x004603e0..0x00460f00 | WorldNode | tools/sharedNOLF/world/WorldNode.cpp | 30 | 17 | 3 | 3 | 6 |
| 0x00460f00..0x00463390 | StdLith | lithshared/stdlith (Lib_StdLith_NOLF.dsp) | 88 | 84 | 0 | 1 | 1 |
| 0x00463390..0x00468d90 | zlib | lithshared/zlib (Lib_ZLib_NOLF.dsp) | 61 | 56 | 1 | 0 | 0 |
| 0x00468d90..0x00469000 | RegMgr | lithshared/regmgr (regmgr60_NOLF.dsp) | 8 | 6 | 0 | 0 | 0 |

Where the boundaries are least sure: EditObjects/EditPoly (0x416c50 and
0x419ca0 hold functions of both), streamsim/RaySelecter (two RaySelecter
pairings at 0x456dc0..0x457010), PreBrush (0x43c280 is SplitPoly's "Bad
poly" template code). 0x431c00 (136 bytes, "Brush | BrushType |
FrictionCoefficient", in node_ops' range) is NOLF's own: nothing of AvP2's
uses those strings.

Corrected by the second round (the table's ranges and counts are the
first link's):

- dtxmgr ends at 0x412770; 0x412770..0x4137b0 is EditBrush's (MakeCap
  0x412770, a brush splitter 0x412e90, another 0x413350, the constructor
  0x4134f0, the destructor 0x4136f0; LoadTBW is 0x413890).
- 0x402760..0x403300 is BasePoly's template code, not bdefs' (bdefs has
  two empty functions the linker folded).
- gettextureflags starts at 0x41f130 (ApplyTextureFlagsToWorld), inside
  geomroutines' range; gettextureinfo at 0x41f4f0 (FindTInfo).
- lightmap_compress ends at 0x422270; lightmap_planes runs to the end of
  g_LMPlanes' static initializer (0x4223d0); LightMapMaker starts after
  it (0x422720 and 0x422b90 are LightMapMaker's).
- PhysicsBlocks starts at 0x4327e0 (MakePBlockNodes_R); 0x4325e0..
  0x4327e0 is parse_world_info's.
- 0x456dc0..0x4574f0 (AreSurfacesSame_RS, GetSurfaceHash_RS 0x457010,
  MergeSurfaces_RS 0x4572a0) is RaySelecter.cpp's, not streamsim's.
- VisLeaves and WorldApprox are one object: 0x45b6b0, 0x45c470, 0x45ccb0,
  0x45ced0 and 0x45d100, in VisLeaves' range, are WorldApprox's.
- Rows the function table splits: CBspGen::SplitPoly's tail is 0x40a4e9;
  MakeWorldModelBSP and SplitPolyList are cut likewise; ClipPortalToPlane
  is 0x41fd40 with 0x420140 its tail; mathlt_impl's 0x430a8c starts mid-
  instruction in RotateAroundAxis (0x430a60). Six more that start mid-
  instruction are in Processor-nolf-screen-exclude.csv.
- The third round corrected the table itself (535 rows merged into the
  row before them in Processor-nolf-screen-exclude.csv, 15 starts added
  in extent-splits.csv; each with its evidence; 2,579 -> 2,217 rows):
  - 43 rows start mid-instruction and 24 begin at an instruction the
    one before falls through into. Most come from an `e8` byte inside
    another instruction (icl's `-0x18(%ebp)` displacement is `e8`, and
    `movl $0,-0x18(%ebp)` reads as `call` to the next instruction) or
    from ASCII in .data/.rdata read as a pointer (RTTI names, the import
    names); six follow `3b c2 73 xx` (`cmp edx,eax; jae`) read as `ret`.
    0x464a19 is inside zlib inflate's jump table.
  - 47 tails reached only by jumps from the function before: every
    deleting destructor's `operator delete` island (17 bytes after the
    `ret`), early-return splits (0x40c515, 0x412408, 0x413d81, 0x41b19a,
    0x414baf), VC6's islands in RegMgr and adler32.
  - 413 unwind actions and 7 catch blocks (read from each function's
    FuncInfo). Our icl objects give a function one symbol up to its
    `$Bn$EH` handler thunk, and the thunk a symbol that runs on to the
    next function; a catch block is a symbol of its own (`catch_T_C`).
    So the funclets before the thunk are the function's, those after it
    the thunk row's (28) or a catch row's (6); the catch rows stay. In
    VC6's PreWorld and StdLith a catch block is the function's.
  - 15 functions the table had no row for, behind an EH handler thunk or
    a function's last jump (lea-nop padding hides them from the `gap`
    rule): 0x401090, 0x413610, 0x414670, 0x414a40, 0x414bf0, 0x415130,
    0x417450, 0x417ee0, 0x41bed0, 0x41c050, 0x4230d0, 0x455ad0, 0x455b70,
    0x455dc0, 0x455e70. Nothing calls, jumps to or points at any of them.

## What AvP2 added later, and what NOLF has that AvP2 doesn't

From the strings (one image's and not the other's), the dialog resources
and the function table:

- No AI volumes (AIVolume, MusicVolume, -aivolumes, the options dialog's
  "Import AI and Music Vol."): FindWorldModel's strings are "Pos" and
  "Terrain" only (0x41cd21).
- No light groups (LightGroup, LightObjects, ModelLight, Object, Position,
  Color, Radius).
- No NoWallWalk surface flag (FindWorldModel's flag list, 0x41d080).
- No .tmp-and-rename save ("Could not remove", "Could not rename", "Try
  again?", "(If you answer no, output file left as ...)"; the CRT's rename
  and strerror messages): "Error opening output file.  Try again?" is all.
- Options: "-LambertianLightmaps " / "LambertianLightmaps" for "-Lambertian";
  "-projectdir \"...\" " quoted; "skipdialog" parsed with the other
  arguments (0x44d30b, globals +0x4d8) and not by the application.
- The options dialog: no slider reference or its "<" ">" buttons, a
  vertical split-weight slider (PreProcessor.rc, decompiled from the image).
- DoProcessing loads in stages of its own: "Loading ED file for BSP.",
  "Loading DAT file for lighting.", "Loading DAT file for PVS generator.",
  "Getting objects from the DAT file.", "ERROR: world object counts don't
  match!" (0x44f630, 0x44fe90); "Making VisBSP leak file." without a file
  name and no "_Vis.lek".
- Light animations: "ERROR: SetVertexColors failed." (0x424760), "      -
  discarding (no data)" for AvP2's "(nothing lit)".
- Kept from AvP2's: terrain (0x4574f0.., "## Creating terrain"), physics
  blocks (0x432a40, "%d bytes for physics, %d blocks"), light animations,
  texture lights (LIGHTDEFS), BlockLight, T-junctions, FastApprox, zones.

## Strings, by the functions that use them (build/strmap.py)

```
0x00403730   2187  *** Bad poly found! > 2 edge splits! (%0.1f %0.1f %0.1f)
0x00404b60    721  Creating temp brushes | Finding intersecters | Sorting intersecters | Merging intersecters | Transferring inside fragments | %d BTW poly splits. | Num Polies: %d.  Joining polies... | %d polies removed. | %d polies left. | !! Empty world model BSP found (%s)
0x00404e3b    293  NoSnap
0x00407970   2836  *** Bad poly found! > 2 edge splits! (%0.1f %0.1f %0.1f)
0x00408a00    564  BSP tree %0.2f%% balanced.
0x0040a5d0    396  %d polies (%d nodes) added for lightmapping.
0x0040b330   1360  *** Bad poly found! > 2 edge splits! (%0.1f %0.1f %0.1f)
0x0040bcf0   2004  *** Bad poly found! > 2 edge splits! (%0.1f %0.1f %0.1f)
0x0040d4e0   1400  *** Bad poly found! > 2 edge splits! (%0.1f %0.1f %0.1f)
0x0040dc10    209  ObjectDLLSetup
0x0040e050    139  TerrainNodeSize
0x0040e0e0    139  WorldNodeSize
0x0040fb10    188  --------------- | Selecting %d problem brushes
0x0040fbd0    131  --------------- | Selecting %d out-of-bounds brushes
0x0040fd5e    318  Found %d problem brushes |   #%d : <Empty>
0x0040fe9c    163  Creating BSP. | Making PhysicsBSP leak file.
0x0040ff3f    500  Removing unseen geometry. | Recreating BSP. | Physics precalculation... | -  %s (%d polies)
0x00410133    125    #%d : %0.1f %0.1f %0.1f
0x0041084f   1317  *** Bad poly found! > 2 edge splits! (%0.1f %0.1f %0.1f)
0x00411180   1891  *** Bad poly found! > 2 edge splits! (%0.1f %0.1f %0.1f)
0x00411ae0    403  Creating BSP. | Removing unseen geometry. | Recreating BSP.
0x00411c73    177  Finding leaves. | -  %s (%d polies) | Making VisBSP leak file.
0x00412408    403  dtx_Create | 66 | 67 | 69
0x004134f0    240  PjA
0x00413604    188  PjA
0x004136f0    143  PjA
0x0041756e    269  %s%d
0x00418080     60  DUMMY
0x0041aaa0     83  Default
0x0041cd21    320  Pos | Terrain
0x0041ce61     49  ERROR: world model without a Name property found.
0x0041cf20    297  MAIN_WORLD
0x0041d080   1132  Solid | Nonexistant | Invisible | Translucent | SkyPortal | FullyBright | FlatShade | GouraudShade | LightMap | Subdivide | HullMaker | AlwaysLightMap | DirectionalLight | Portal | SkyPan | Additive | TimeOfDay | TerrainOccluder | VisBlocker | NotAStep
0x0041d5b0     48  DetailLevel
0x0041d7e0    160  %f
0x0041d8c0    164  %f %f %f | true | false
0x0041d970     96  %f %f %f
0x0041f1d0    164  Imported %d texture flags
0x0041f274     54  ** Couldn't find any textures. | ** (Is project path set?)
0x0041f540    751  center | dirlight
0x0041f8b0    413  %s\\%s | rb
0x0041fa4d    116  LIGHTDEFS
0x00420be0    778  Creating leak file %s | OutsideDef | ** Couldn't find a path from inside to outside. | wt | %f %f %f %f %f %f\n | ** Unable to open leak file %s.
0x00420eea     90  ** Couldn't find an outside leaf. | ** (Or the world is sealed correctly). | ** Couldn't find an inside leaf for leak file.
0x00423c70   1262  Ambient light (%.1f, %.1f, %.1f)
0x00424760    188  LightAnim_BASE | ERROR: SetVertexColors failed.
0x00424820    204  Warning: couldn't load object.lto for light animations
0x00424966    548  Can't create any threads to process lighting! | Warning: could only create %d threads for lighting.
0x00424ba0    272  WARNING: Invalid poly lightmap size, clamping.
0x00425dc2     71  ** Unable to get texture lights. | ** (Is project path set?)
0x00425e09    251  Created %d texture lights.
0x00426490   1747  Light | DirLight | LightColor | InnerColor | OuterColor | Pos | Rotation | ClipLight | FOV | LightRadius | BrightScale | StaticSunLight | Bias
0x00426e40    170  BlockLight
0x00428b40    112  %s\\object.lto
0x0042a230    407  LAnim - %s (%d frames) |       - discarding (no data) |       - %d bytes
0x00431c00    136  Brush | BrushType | FrictionCoefficient
0x004325e0    308  AmbientLight
0x00432720     92  LightTableRes
0x00432780     92  PBlockSize
0x00432a40   5250  %d bytes for physics, %d blocks (%d average nodes per block, %d max)
0x0043c280   2480  *** Bad poly found! > 2 edge splits! (%0.1f %0.1f %0.1f)
0x0043cef0   1555  NoSnap | ** Found an out-of-bounds brush
0x0043d503    571  ** Found a ZERO THICKNESS BRUSH (%d) | ** Unable to generate a plane (%d)
0x0043e060    107  GameStartPoint | StartPoint | Light | DirLight
0x00441340     12  @:H
0x00441660    304  PreProcessor | No world file specified. | %s.dat
0x00441790    469  Monolith Productions | Lithtech | 2.0 | PreProcessor | Priority
0x00441b80    240  Priority
0x00441cf0     12  H=H
0x00441d10     12  H=H
0x004424c0    179  %s\r\n
0x004426c0    168  Approximation
0x00442810    111  PreProcessor
0x00442920    120  Default
0x00442ac0     39  Full Optimization
0x00442bd0    189  textures\\default.dtx
0x004431f7    120  o3D
0x00446e44     29  DnD
0x00446f6d      7  DnD
0x00446f74    371  QoD
0x0044b69a     61  bad allocation
0x0044cec0   1784  LeakFile | PortalFile | MaxBounds | RaySelect | SelectProblemBrushes | SelectOutOfBoundsBrushes | SelectHullProblemBrushes | RemoveProblemObjects | TextureFlags | ProjectDir | NoPortals | bsp | replacetextures | numthreads | light | nocliplight | skipdialog | optimize | fast | LightAnims | onlylightverts | LambertianLightmaps | maxdetail | splitweight | logfile | ignorehidden | 
0x0044d5c0    164  %s_%d.log | rt | wt
0x0044d670    100  %s\r\n
0x0044d810    502  %s.ed | rb | * Object '%s' (class '%s') has an invalid position (%.2f %.2f %.2f). | Can't open ED file: %s.
0x0044da06    239  Invalid ED file version (%d). | The ED file is corrupt (error code %d).
0x0044dbc0    238  %s.ed | wb | Can't open ED file %s for writing.
0x0044dcd0    288  rb | Error loading %s. | DAT file %s is corrupt.
0x0044ddfa     37  DAT file %s is an invalid version.
0x0044de20    149  wb | Error opening output file.  Try again?
0x0044deb5     92  ERROR IN CPreWorld::SaveFile!
0x0044df20    373  Num detail nodes: %d
0x0044e0b0    147  Error making hulls.
0x0044e190    384  Error making PVS.
0x0044e31c     61  ERROR: No visibility BSP!
0x0044e3c0    308  --------------- | SelectHullProblemBrushes: Selected %d brushes.
0x0044e4f4     98  Number of (unseen) polies removed: %d
0x0044e660   1335  invisible.dtx | Effect | Name
0x0044ebb0    343  EffectParam
0x0044ef30    828  Done in %.2f minutes | Number of input polies: %d | Number of input vertices: %d | Number of output polies: %d | Number of output vertices: %d | Tree depth: %d | Memory for vis lists: %d | Number of (unseen) polies removed: %d | Number of leaves: %d | Number of visible leaves: %d | Lightmap data size: %d | Lightmap compression ratio: %.2f | Number of objects: %d | Light table si
0x0044f270    107  MaxLMSize
0x0044f2e0     73  LMGridSize
0x0044f350    207  Merging %d brushes. | --------------- | Found an invalid (unprocessable) brush!
0x0044f420    465  Creating physics BSP | PhysicsBSP | Creating visibility BSP | VisBSP
0x0044f630   1094  Must have at least one brush! | --------------- | RaySelect: Did %d ray selections | ERROR: world object counts don't match! | WorldTree nodes: %d | WorldTree depth: %d | WorldTree root size: %.2f
0x0044fae0    200  %d 
0x0044fbb0    724  wt | File Version: %d\n\n | Leaves: %d\n | \n\nLeaf #%lu\n\n |   Polies: %d\n |     Connection: %lu\n |     Verts: %d\n |       %f %f %f\n |     Visibility: \n |     Visibility:  |     Center: %f %f %f\n
0x0044fe90   1997  Processing %s.ed | Date: %s, Time: %s | This machine has %d processor%s. | Using %d threads. | Max detail level: %d. | Getting objects from ED file. | --------------- | Removing problem objects | Replacing textures... | Replaced %d textures | ------------------- | Loading ED file for BSP. | Getting objects from the DAT file. | Max Lightmap Size: %.2f | Lightmap Grid Size: %.2f |
0x0045065d     69  Internal error!  (Old file format?) | Ran out of memory!
0x00452de0    548  -replacetextures  | -textureflags  | -bsp  | -ignorehidden  | -FixTJunc  | -light  | -nocliplight  | -lightanims  | -onlylightverts  | -LambertianLightmaps  | -optimize  | -fast  | -logfile  | -projectdir " | "  | -splitweight %f 
0x00453670    698  rt | processor.opt
0x00453a20    457  wt | processor.opt | %s\n
0x00453ca0     12  H=H
0x00454030     60  p@E
0x004540e0     64  %f %f %f
0x00454120     48  PAE
0x00454770     52  @GE
0x00454880     56  PHE
0x004548c0     48  p@E
0x00454910     52  %f %f %f
0x00455050    600  RaySelecter | Rotation
0x004555b0    299  textures\\invisible.dtx
0x00455b64    303  rb
0x00456460    141  0kE
0x004566e0    103  `kE
0x00456940    110  QoD
0x00456ab0     16  DnD
0x00456ac0     11  DnD
0x00456cd0    105  `kE
0x004574f0    139  TerrainSubDivSize
0x00457636    311  ****ERROR: can't get node path!
0x00457810   1999  ## Creating terrain '%s'. | %d surfaces merged. | %d terrain polies merged. | %d polies left. | ** Found terrain surface(s) with Lightmap TRUE.  Lightmaps will overfl | %d terrain sections (avg. %.1f polies/section) | %.1fx poly overlap (%d extra polies) | Generating %d terrain BSPs | CBspGen::GenerateBspTree returned FALSE
0x004589e0   1423  *** Bad poly found! > 1 edge split! (%0.1f %0.1f %0.1f)
0x00459800   3053  Fixing T-Junctions on %d polys... | *** WARNING *** WARNING *** WARNING *** | ** Polygon split processing terminated | ** due to full node tree.  Get rid of a | ** large number of T-Junctions and | ** try again. | ** Unable to complete T-Junction processing | ** due to invalid geometry.  Coordinates of | ** first point of current polygon: | **    %0.4f,%0.4f,%0.4f | Added %d pol
0x0045aad0    255  InsideDef
0x0045d5c0    264  FastApproxArea | Marked %d out of %d leaves as FastApprox
0x0045d920    538  Zone
0x0045db3a     66  Can't find world model for zone '%s'
0x0045fe00    163  CreateNodes, depth: %d, nNodes: %d
0x00461130     16  P.F
0x00461140      7  P.F
0x00461e90    398  0GC | P.F
0x00462540    234  1.0.4
0x00462a50    245  1.0.4
0x00462b80    161  0GC | P.F
0x004633c0    492  1.0.4
0x004644f0    270  1.0.4
0x00464620   1017  unknown compression method | invalid window size | incorrect header check | incorrect data check | need dictionary
0x00466bf0   3368  invalid block type | invalid stored block lengths | too many length or distance symbols | invalid bit length repeat
0x00467960     85  oversubscribed dynamic bit lengths tree | incomplete dynamic bit lengths tree
0x00467e80    232  oversubscribed literal/length tree | incomplete literal/length tree
0x00468150   1936  invalid literal/length code | invalid distance code
0x00468a40    834  invalid distance code | invalid literal/length code
0x00468d90    284  ware | Software
```
