# LithTech for Blender

Import and export of LithTech models and textures in Blender (5.2 LTS and
later), starting with the 2000-2001 engine (No One Lives Forever, Aliens
versus Predator 2: `.abc` model version 12, DTX textures version -5), and
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
- `tests/`: `roundtrip_abc.py` and `roundtrip_dtx.py` read and write back
  every shipped model and texture; `blender_roundtrip.py` (run inside
  Blender) imports and exports every model through the extension.

Game data is never committed; tests read it from the installed games
(`~/src/rez/games`) through `~/src/tools/rezextract.py`.
