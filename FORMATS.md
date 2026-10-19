# Format compatibility: which tool builds can make files for which game

Checked 2026-10-19 to settle how many tool builds the project needs. Two
kinds of evidence: the versions in every shipped asset
(`scripts/fmtversions.py`, which reads each game's `.rez` files), and the
version each engine accepts and each tool writes, read from the binaries
(the check that prints the error, and the constant written next to it) or
from the engine reconstructions in nolfsrc and nolf2src.

## Shipped assets

| Game | Worlds (`.dat`) | Models | Textures (`.dtx`) |
|---|---|---|---|
| NOLF (GOTY, US) | v66 (106) | `.abc` v12 (692; one v11, two v9) | −5 |
| AvP2 | v70 (68) | (none in the retail `.rez` files read) | – |
| AvP2 Primal Hunt | v70 (53) | `.abc` v12 (59) | −5 |
| NOLF2 (US) | v85 (62) | `.ltb` type 1 v9, model v23 (746; one v18) | −5 |
| Contract J.A.C.K. (US) | v85 (40) | `.ltb` type 1 v9, model v23 (691; two v18, one v22) | −5 |
| Tron 2.0 (US) | v85 (60) | `.ltb` type 1 v9, model v23 (860; four v18-v22) | −5 |

(`.ltb` type 5 entries are compiled render styles, a separate format.)

## The older engine (NOLF, AvP2)

- **Compiled worlds differ.** NOLF's engine refuses any world that isn't
  version 66: `w_ReadWorldHeader` in `nolfsrc/src/engine/server/commonmodel.cpp`
  returns 0 unless the version is 66. AvP2's are 70. Each Processor has the same
  check-then-write code: NOLF Tools 001's uses 0x42 (66), AvP2 1.0's uses
  0x46 (70) and nothing writes 66. **So NOLF needs its own Processor**
  (NOLF Tools 001, `Processor.exe`, 2001-01-03, VC6 SP3 8447, a held
  compiler). AvP2 Update 1/2 has no Processor of its own; AvP2 1.0's
  (2001-10-03, VC6 SP5 8966) is the AvP2 one.
- **World sources are shared.** Every `.ed` in both tool releases (NOLF's
  sample worlds and 200+ prefabs, AvP2's sample and tutorial worlds) starts
  with version 1249. (Jupiter DEdit's `.lta` writer still prints
  `( versioncode 1249 )`.)
- **Models and textures are shared:** `.abc` v12 and DTX −5 in both games.
- So one DEdit and one ModelEdit (AvP2 Update 2's Talon builds) can serve
  both games, plus two Processors. **Not yet tested:** that a level the
  Talon DEdit saves goes through NOLF's Processor. The Talon DEdit adds
  `.lta` import, and imported geometry might use something the January 2001
  Processor's `.ed` reader lacks. This needs a run under Wine. The
  Processor's own `.ed` check is a loader status, not a version constant
  next to the message, so it isn't read here.

## The newer engine (NOLF2, Contract J.A.C.K., Tron 2.0)

- **Worlds:** NOLF2 and CJ ship v85, and the engine accepts exactly
  `CURRENT_WORLD_VERSION` 85. Every `WorldPacker.dll` (NOLF2 Toolkit, Tron,
  CJ, Build 69) has the same 0x55 (85) constant where Build 69's
  `PCWorldPacker.cpp` writes `CURRENT_DAT_VERSION`.
- **Models: Build 69 writes a version the 2003 games never shipped.**
  `lta2ltb_d3d.cpp` writes `MODEL_FILE_VERSION`. In `Model_Packer.exe` that is
  `mov ebx,0x17` (23) in SDK build 51, the NOLF2 Toolkit, Tron and CJ, and
  `mov ebx,0x19` (25) in Build 69. The NOLF2 (toolkit `lithtechdev.exe`,
  2003-02-26) and CJ engines only check the minimum (`cmp eax,0x17` / `jae`
  before "fileversion %d is not supported"; SDK build 51's checks 18). But
  version 24 changed how per-model OBBs are stored (`MIN_MODELOBB_FILE_VERSION`
  in Build 69's loader), so a v25 model with OBBs would be misread by a 2003
  engine. **So Build 69's tools can't be used for these games as they are**,
  at least not `Model_Packer`. A CJ-era build (version 23) is needed,
  which the planned back-port gives.
- **Tron 2.0 needs no build of its own.** Its retail engine
  (`Lithtech.exe` from the Tron 2.0 (USA) discs, archive.org `tron20usa`,
  SHA-1s match) has the same model check (`cmp eax,0x17` / `jae`) and
  ships v85 worlds and v23 models, like NOLF2 and CJ. So CJ's tools make
  Tron files too, and Tron's own VC6 8799 build isn't needed. DEdit takes
  each game's object classes from that game's `Object.lto`, so nothing
  game-specific is built into it.
- The `MODEL_FILE_VERSION 25` in the
  nolf2src and cjacksrc engine trees is Build 69's header carried over, not
  what their retail engines check.

## Consequences for the plan

| Game | DEdit / ModelEdit / exporters | World compiler | Model packer |
|---|---|---|---|
| NOLF | AvP2 Update 2 (Talon) | **NOLF Tools 001 Processor** (writes v66) | – (ModelEdit saves `.abc`) |
| AvP2 | AvP2 Update 2 (Talon) | AvP2 1.0 Processor (writes v70) | – |
| NOLF2, CJ | CJ tools (Build 69 source, back-ported) | CJ `WorldPacker.dll` (v85) | CJ `Model_Packer` (v23) |
| Tron 2.0 | CJ tools | CJ `WorldPacker.dll` (v85) | CJ `Model_Packer` (v23) |
