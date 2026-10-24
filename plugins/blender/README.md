# LithTech for Blender

Import and export of LithTech models and textures in Blender (5.2 LTS and
later):

| Game | Models | Textures |
|---|---|---|
| No One Lives Forever, Aliens versus Predator 2 (and Primal Hunt) | `.abc`, versions 9 to 12 | DTX version -5 |
| No One Lives Forever 2, Contract J.A.C.K., Tron 2.0 | `.ltb`, model file versions 18 to 25 | DTX version -5 |
| No One Lives Forever (PS2) | | DTX version -5, 32P (palette alpha as the PS2 draws it) |

meant to grow to every LithTech-era game.

- `io_scene_lithtech/`: the Blender extension (`blender_manifest.toml`),
  mapping models onto armatures, meshes, vertex groups and actions, and
  textures onto images. See `docs/BLENDER.md` for installing and using it.
- `io_scene_lithtech/lithtech/` (also reachable as `lithtech/`): the file
  formats as a plain Python library, no Blender: read a file into objects,
  write them back byte for byte. It lives inside the extension so it ships
  with it.
- `docs/`: the formats as found, with the evidence for each field (the
  shipped tools in `../../talon`, the engine reconstructions, Build 69).
- `tests/`: `roundtrip_abc.py`, `roundtrip_ltb.py` and `roundtrip_dtx.py`
  read and write back every shipped model and texture (`extract_abc.py` and
  `extract_ltb.py` fill the asset cache first); `blender_roundtrip.py` and
  `blender_roundtrip_ltb.py` (run inside Blender) import and export every
  model through the extension. `roundtrip_ps2_dtx.py` and
  `blender_roundtrip_ps2_dtx.py` do the same for the PS2 textures (from
  `out/assets/ps2`, the extracted PS2 discs).

Game data is never committed; tests read it from the installed games
(`~/src/rez/games`) through `~/src/tools/rezextract.py`.
