# LithTech tools: what exists, and what built it

Inventory of the public (and leaked-to-public) LithTech editing tool
releases for NOLF, NOLF2, Contract J.A.C.K., Tron 2.0 and AvP2, made
2026-10-19. `inventory.tsv` has one row per PE image: path,
size, SHA-256 prefix, PE time stamp, linker version, compiler from the Rich
header, runtime DLLs, 3ds Max / Maya plugin, FileVersion, CodeView PDB path.
Regenerate it with:

```sh
E=extracted
python3 -I scripts/inventory.py ~/src/tools \
    nolf-tools-001=$E/nolf-tools-001/zip \
    avp2-tools=$E/avp2-ph/x/avp2tools \
    avp2-tools-update1=$E/avp2-ph/x/avp2tools_update \
    avp2-tools-update2=$E/avp2-ph/x/avp2tools_update2 \
    jupiter-b51=$E/jupiter-b51/x \
    tron2-tools-official=$E/tron2-tools-official/x \
    tron2-tools-1.2a=$E/tron2-tools-1.2a/x nolf2-toolkit=$E/nolf2-toolkit/x \
    cjack-tools=$E/cjack-tools/x jupiter-b69=$E/jupiter-b69/x > inventory.tsv
```

`downloads/` and `extracted/` are not committed (`.gitignore`).

## The headline

**The Jupiter tools don't need decompiling: their source is public.** The
"Jupiter System Enterprise Edition, Build 69" package (Touchdown
Entertainment, 2006, archive.org; the same InstallShield set as the
`nolf2_gpl_src` item) has the full tool source: DEdit, ModelEdit, FxED,
WinPacker/WorldPacker, PreProcessor, LithRez, Model_Packer, RenderStylesEditor,
ButeEdit, WaveEdit, LTC/WinLTC, lipcompiler, DtxView and the rest, **and the
3ds Max (3-7) and Maya (4.0-7.0) plugins**, with VS.NET 2003 solutions
(`Engine/tools/Tools.sln`, `Engine/tools/Plugins/*.sln`, `*.vcproj`). It
also ships 2006 VC7.1 builds of all of them, which are the newest public
Jupiter tool binaries. `~/src/fear/Dev/Source/tools` is the same tree minus
the plugins (565 vs 713 `.cpp`).

The newest Jupiter tools released *with a game* are Contract J.A.C.K.'s
(`PublicToolsSetup.exe` on CJ disc 1, Oct 2003), built with VC7.1 13.10.3077,
the compiler cjacksrc already holds (`cjacksrc/toolchains/msvc-7.10-3077`).
They are close to Build 69: string-set similarity with Build 69's builds is
0.95 for DEdit, 0.94 ModelEdit, 0.92 FxED (SDK build 51's DEdit: 0.24). So a
byte-matching reconstruction of the CJ tools would be a back-port of Build
69's source, the way cjacksrc and nolf2src back-port its engine, not a
decompilation from nothing.

The NOLF2 Toolkit's tools (built 2003-02-27; DEdit 2003-03-19) are equally
close: DEdit 0.95 to Build 69. They were built with VC6 SP5 (12.00.8966),
which `toolchains/msvc-6.00-8966` holds, so they are the cheapest target to
byte-match. Tron's (2003-03-28) were built with VC6 8799, which no
toolchain here has. Tron's DEdit scores only 0.27 on the string check, but
its `.text` is almost the same size as NOLF2's (0x1aaf25 vs 0x1ac0f5); its
`.rsrc` is about half NOLF2's (0x7ed50 vs 0xef150). So it's the same code
with fewer strings in its resources, and the string check undercounts it.
Tron's and NOLF2's ModelEdit are identical by this measure (1.00).

Release order for Jupiter: SDK b51 (2002-03) → NOLF2 Toolkit (2003-02/03)
→ Tron (2003-03) → CJ (2003-09/10) → Build 69 (2006-03).

**The LithTech 2.x / Talon tools (NOLF, AvP2) have no source.** Those
are the real decompilation targets. Their world formats (`.ed` source,
older `.dat`) and models (`.abc`) differ from Jupiter's (`.lta`/`.ltc`,
`.ltb`), so the Jupiter tools can't edit NOLF levels. The newest of them
is **AvP2 Tools Update 2** (May 2002), from the AvP2 Primal Hunt disc. It
has the "Talon" DEdit and ModelEdit (Dec 2001), which can also import
`.lta`, and Max/Maya exporters that write Talon `.lta`. See "LithTech 2.x /
Talon tools" below.

## Sources

| Item | Where | SHA-256 | Unpacked into |
|---|---|---|---|
| NOLF Tools 001 (Jan 2001) | NOLF GOTY disc 2, `Game/NOLFTools001.exe` (zip SFX around an InstallShield 5 setup; files stored plain) | `b2a13a39…71d1` | `extracted/nolf-tools-001/zip/` |
| LithTech Jupiter SDK build 51 (Mar 2002) | `~/src/nolf2src/rez/isos/sdk/LithEngine.iso` (archive.org `lithengine`) | `8b1a543d…e939` | `extracted/jupiter-b51/x/` (all groups; nolf2src's rez installs only "Source Code") |
| NOLF2 Toolkit (`si_nolf2_tools_en_10.exe`, released May 2003) | supplied by the user (ModDB) | `26b78f78…f85c` | `extracted/nolf2-toolkit/x/PatchEN/` |
| TRON 2.0 tools, official `tron_tools.zip` (zip dated 2003-10) | supplied by the user | `386407ee…f063` | `extracted/tron2-tools-official/x/` |
| TRON 2.0 Editing Tools v1.2a | TronFAQ / LDSO Google Drive link, `tron20_tools_v1x2a_setup.zip` (Setup Factory 6) | `1f4a49a1…1895` | `extracted/tron2-tools-1.2a/x/` |
| Contract J.A.C.K. Public Tools (Oct 2003) | CJ (USA) disc 1, `PublicToolsSetup.exe` (InstallShield SFX; identical on the EU disc) | `7aa7d5ed…ec24` | `extracted/cjack-tools/x/` |
| AvP2 Editing Tools v1.0 (2001-12-14), Update 1 (2002-01-31), Update 2 (2002-05-06) | AvP2 Primal Hunt disc (archive.org `avp2_PH`, `AvP 2 Primal Hunt Expansion.bin`, a raw Mode 1 image), `Tools/avp2tools.exe`, `avp2tools_update.exe`, `avp2tools_update2.exe` (InstallShield SFX) | `828bc1c9…669e`, `7c3fd1f3…75ab`, `485a8ea0…5662` | `extracted/avp2-ph/x/{avp2tools,avp2tools_update,avp2tools_update2}/` |
| Jupiter System Enterprise Edition Build 69 (2006) | archive.org `Lithtech-Jupiter-System-Enterprise-Edition-Build-69` | `d0d05c25…b5a6` | `extracted/jupiter-b69/x/` |

The Tron v1.2a installer is a fan repackaging (2011+) of the official 2003
`tron_tools.zip`. Every Monolith binary in it is byte-identical to the
official zip's (75 files identical; 3 sample render-style `.lta` files
differ). Those binaries are dated 2003-03-28 (Processor 2002-10-11,
lipcompiler 2002-06-11). The fans added WinRez LT Studio, XN
Resource Editor, Build 69's 2006 LithRez (in the installer header), the
CJ/NOLF2/AvP2/FEAR documentation and sample maps. `scripts/sfextract.py`
(with `scripts/blast.c`, a PKWARE DCL decoder) unpacks it, CRC-checking
every file.

**The AvP2 tools installers are verified clean.** They are byte-identical on
three images of the Primal Hunt disc:

- `Redump.org35760`, Primal Hunt (Europe): the `.bin` matches redump.org
  disc 35760 (SHA-1 `b47b0750…747d`).
- archive.org `aliens-versus-predator-2-primal-hunt-us-2002-win-1.0.2.2`,
  Primal Hunt (USA), a DiscImageCreator dump (`CD_DIC.rar`): the `.bin`
  matches the dump's own `.dat` (SHA-1 `f70213ed…fc51`).
- `avp2_PH`: the US disc with a `Razor1911/` folder (Team Razor's crack and
  NFO, 2002-08-14) added. Apart from that folder its file listing is the US
  dump's, and its root `PrimalHunt.exe` is the retail one (the same on all
  three images).

The EU and US discs differ only in their InstallShield cabinets. Unpacked
into `extracted/avp2-ph-eu/` and `extracted/avp2-ph-us/`; the archives
are in `downloads/avp2-ph-eu-redump/` and `downloads/avp2-ph-us-dic/`.
Unpacking the `.7z` and `.rar` took the official 7-Zip 23.01 Linux binary
(`7z2301-linux-x64.tar.xz`, SHA-256 `23babcab…f318`, from 7-zip.org),
kept in the session scratchpad, not installed. The archive.org item
`aliens-versus-predator-2-gold-edition` is a 2016 KaOs repack, not disc
images, and isn't useful.

The two AvP2 game discs
in the same item (`AvP2 CD1.iso`, `CD2.iso`; SHA-1s match archive.org's)
have no tools at all: the retail setup's 34 file groups are game, launcher,
worlds, DirectX, GameSpy and InstallShield support.

## LithTech 2.x / Talon tools

| Tool | NOLF 001 (2001-01) | AvP2 v1.0 (built 2001-10) | AvP2 Update 1 = Update 2's DEdit/ModelEdit (2001-12) | AvP2 Update 2 plugins (2002-03-28) |
|---|---|---|---|---|
| DEdit | 2001-01-04, VC6 8447 | 2001-10-03, VC6 8966 | 2001-12-06, "Talon": VC6 Processor Pack 9044 ×230 | – |
| ModelEdit | 2000-09-18, VC6 8168 | 2001-10-03, VC6 8966 ×64 (plus 85 objects of statically linked MFC, 8569) | 2001-12-19, "Talon": VC6 Processor Pack 9044 ×73 | – |
| Processor | 2001-01-03, VC6 8447 | 2001-10-03, VC6 8966 | – | – |
| LithRez | 1998-11-07, pre-VC6 | same file | – | – |
| Max | LithExport/LandExport (2000-09, 8168) | LithExport/LandExport (2000-08, 8447); MaxWorldExport/Import 30/40 (2001-10, 8966) | – | MaxWorldExport/Import, MaxModelExport, MaxDTXLoader 30/40, 8966 |
| Maya | – | LithMayaExport.mll (1999, 8168); MayaWorldExport 30/40 (2001-10, 8966) | – | MayaWorldExport, MayaModelExport 30/40, IMFLithDTX, 8966 |

- DEdit string-set similarity: NOLF 001 vs AvP2 v1.0 is **0.98** (nearly
  the same program); either vs Talon is 0.57-0.59; Talon vs Jupiter SDK
  build 51 is 0.27; NOLF's vs NOLF2's is 0.10. So NOLF's DEdit and AvP2
  1.0's are nearly the same program. Talon sits between them and Jupiter.
- Every compiler here is held: `msvc-6.00-8168`, `-8447`, `-8966`, and for
  the Talon DEdit and ModelEdit, `msvc-6.00-9044` (VC6 SP5 with the
  Processor Pack, back end 13.00.9044). That one is checked: a test
  compile with it gives `@comp.id` 0x31/9044 for C++ and 0x30/9044 for C,
  the Talon images' Rich rows. The 8569 rows in these images are MFC's
  objects, as `toolchains/COMPILERS.md` notes for NOLF.exe, not
  Monolith's code.
- What helps: nolfsrc already rebuilds the LithTech 2.x engine
  (`nolfsrc/src/engine`) and the `lithshared` libraries (rezmgr,
  dibmgr, ButeMgr, ...) that these tools share with it. The AvP2 source
  release (`~/src/avp2/proj/LT2`) has LT2's SDK headers and `lithshared`,
  with debug libraries. And Jupiter DEdit is a descendant of this one, so
  Build 69's source is a guide to its structure.

## Tool binaries by release (Jupiter)

Compiler is the Rich header's main C++ entry. `inventory.tsv` takes the
C++ row with the most objects, and in a program that links MFC statically
that row can be MFC's (VC6 8569), not Monolith's code. Check such images
with `~/src/tools/richhdr.py`. For VC6: 8168 = RTM, 8447 = SP3,
8799/8797 = post-SP4 (no toolchain held), 8966 = SP5 (held). 9178 = VC7.0
(VS.NET 2002), not held. 3077 = VC7.1 (VS.NET 2003) RTM, held by cjacksrc.
4035 = a later VC7.1 build.

| Tool | NOLF 001 (2001) | SDK b51 (2002-03-15) | NOLF2 Toolkit (2003-02-27) | Tron (2003-03-28) | CJ (2003-09/10) | Build 69 (2006-03) |
|---|---|---|---|---|---|---|
| DEdit | 2001-01-04, VC6 8447 | VC6 8966 | 2003-03-19, VC6 8966 | VC6 8799 | 2003-10-08, VC7.1 3077 | VC7.1 3077 |
| ModelEdit | 2000-09-18, VC6 8168 | VC6 8966 | VC6 8966 | VC6 8799 | VC7.1 3077 | VC7.1 3077 |
| Processor / WinPacker+WorldPacker | Processor 2001-01-03 8447; TProcessor 1998 (pre-VC6) | Processor 8966 | winpacker + WorldPacker.dll 8966 | Processor 2002-10 8799; winpacker + WorldPacker.dll 8799 | winpacker + WorldPacker.dll 3077 | winpacker + WorldPacker.dll 3077 |
| LithRez | 1998-11-07, pre-VC6 | 8966 | 8966 | 8799 | 3077 | 3077 |
| FxED | – | 8966 | 8966 | 8799 | 3077 | 3077 |
| RenderStyles Editor/Viewer/Packer | – | VC7.0 9178 | VC7.0 9178 | VC7.0 9178 | VC7.0 9178 | VC7.1 (4035 / 3077) |
| Model_Packer, LTC, WinLTC, WaveEdit | – | 8966 | 8966 | 8799 | 3077 | 3077 |
| Max exporters (model, world), world importer, DTX loader | LithExport.dll / LandExport.dll (2000-09, 8168; LithExport has a debug PDB path) | Max 3 & 4, 8966 | Max 3 & 4, 8966 | Max 3 & 4, 8799 | Max 3 & 4, 3077 | Max 3, 4, 5, 6, 7, 3077 |
| Maya model/world exporters, IMF DTX | – | Maya 3 & 4 ("30"/"40"), 8966 | same, 8966 | same, 8799 | same, 3077 | Maya 4.0, 4.5, 5.0, 6.0, 6.5, 7.0, 3077 |
| Photoshop DTX (`DTXFormat.8bi`) | – | – | 2000-09-25 VC6 8168, debug build | same file | same file | 2001-08-03 VC6 8799 |
| Other | – | ButeEdit, PropHelpGen, dtxutil, ltaview, ltbhead, LTAsk, pma, HeightMaptoNormalMap (VC7.0) | lipcompiler, TSCompiler (VC6 8168), LTDMTest, ServerDir.dll, `lithtechdev.exe` (2003-02-26, 8966), the Demolition mod's game DLLs | lipcompiler, LTDMTest | TSCompiler, LTDMTest, `lithtechdev.exe` (CJ dev engine) | everything in b51 plus DtxView, DtxOptimizer, DtxShellEx, TSCompiler, lipcompiler, vcvars, ServerDir.dll |

Notes:

- The LithTech 2.x tools are stripped: no PDB paths, except
  TProcessor's (`F:\Proj\DE\DEdit\TProcessor\…`) and the Max exporter's
  (`D:\Proj\Nolf\lt2\tools\model\SoftExport\Debug\LithExport.pdb`, a
  debug build with asserts, which helps).
- SDK build 51 also has its own TO2 (NOLF2 prerelease) engine and game
  binaries, as nolf2src's `docs/engine/anatomy.md` already records.
- The NOLF2 Toolkit also has 1000+ prefabs, the NOLF2 game source
  (`Tools/Source/Source.zip`) and a few world sources under
  `Game/Worlds/Retail*`, among them `c03s03.ltc`.
- Build 69's `Engine/built/Release` has `Lithtech.pdb` and `ClientFx.pdb`
  paths but no `.pdb` files.

## Still missing

(Nothing needed for the plan. `FORMATS.md` shows CJ's tools serve Tron
2.0, so VC6 8799, Tron's own compiler, isn't needed.)

- Lesser leads not fetched: archive.org `lith-tech-ftp-dump.-7z` (61 MB; its
  listing shows only `lithtech_lg.avi` and one unnamed file),
  `sudden-attack-full-source-dev-tools` and `Combat-Arms-tools-source-code`
  (leaked later Jupiter-derived trees), `shogo-mad-dev-tools` (LithTech 1.0).
