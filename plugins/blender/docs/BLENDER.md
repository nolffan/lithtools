# The Blender extension

`io_scene_lithtech` imports and exports LithTech models (`.abc`, versions 9
to 12) and textures (`.dtx`, version -5) in Blender 5.2 LTS and later.

## Installing

Build the package and install it (or drag the zip onto Blender):

```
blender --command extension build --source-dir io_scene_lithtech --output-dir dist
blender --command extension install-file -r user_default -e dist/io_scene_lithtech-0.1.0.zip
```

Then use **File > Import / Export > LithTech Model (.abc)** and **LithTech
Texture (.dtx)**, or drop an `.abc` onto the viewport. In the Image Editor,
**Image > Open / Save as LithTech Texture** does the same for textures.

## Models

| LithTech | Blender |
|---|---|
| node tree | armature; one bone per node, at its bind matrix |
| piece | mesh object parented to the armature, with an Armature modifier |
| vertex weights | vertex groups named after the bones |
| UVs (one per triangle corner) | the `UVMap` layer |
| vertex normals | custom normals |
| LODs 1..n | hidden objects `<piece>_LOD<n>` in a `<model> LODs` collection |
| skin slot | material `<model>_skin<n>`, with the DTX if one was found |
| sockets | arrow empties parented to their node's bone |
| animations | actions (fake user), linear keys at the stored times |
| keyframe strings | the action's pose markers |
| child models' animations | actions too, marked `lt_child` (not exported) |

**Space.** LithTech is left-handed with Y up and models face +Z. The
importer swaps Y and Z (which also fixes the handedness and turns the
triangle winding round), so models stand on Blender's Z up and face +Y.
*Face Front* (on by default) turns the armature object 180° to face -Y, as
Blender's front view expects. The exporter writes the model in the
armature's own space, so the object's transform never reaches the file.

**Scale.** LithTech units have no fixed size; a NOLF character is about
100 units tall. Import *Scale* is Blender units per LithTech unit (0.018
gives a 1.8 m character); export *Scale* is the inverse and defaults to
undo whatever the model was imported at.

**Skins.** The file names no textures: the game's attribute files pick
them. The importer looks for `SKINS/<model>.dtx` beside the `MODELS`
directory (NOLF and AvP2 keep `CHARS/MODELS/X.abc` with
`CHARS/SKINS/X.dtx`), then `<model>_*.dtx` for the other slots, in the
model's directory and in *Skin Directory*. A wrong match is fixed by
picking another image in the material.

**Alpha.** Skins often use alpha as a mask, or leave it at 0, so the image
keeps it (`Channel Packed`) without making the material see-through.

### What is kept for export

LithTech data with no place in Blender is kept as custom properties, all
named `lt_*`:

- armature object: `lt_version`, `lt_command_string` (the model's command
  string: shadows, `NormalRef` and so on), `lt_internal_radius`,
  `lt_lod_distances`, `lt_scale`, `lt_save_index`, and JSON for the weight
  sets, the child models and their animation bindings;
- bones: `lt_index` (the order nodes are written in) and `lt_flags`;
- piece objects: `lt_piece`, `lt_piece_index`, `lt_lod`,
  `lt_texture_index`, `lt_specular_power`, `lt_specular_scale`,
  `lt_lod_weight`;
- actions: `lt_dims` (the box), `lt_translation`, `lt_interpolation_ms`,
  `lt_weight_set`, `lt_index`, and the keyframe strings exactly (pose
  marker names stop at 63 bytes and sit on whole frames);
- meshes: `lt_replacement` (each vertex's collapse target), `lt_normal`
  (the stored normals, since Blender keeps custom normals only to about
  1e-3 and can't hold a zero one) and `lt_import_normal` (the corner
  normals at import, so the exporter can tell which ones were edited).
  Vertices whose weights don't follow from their position (some LODs, and
  biases that don't sum to 1 or name a node twice) also keep their stored
  weights while they stay where they were.

Everything has a default, so models made in Blender export as well: any
mesh deformed by the armature is a piece (one per material), vertex groups
named after bones are the weights, empties on bones are sockets, and every
action that animates the bones is an animation, keyed wherever any of its
F-curves has a key. With no actions the model gets one animation, `base`,
at the rest pose.

### Round trip

`tests/blender_roundtrip.py` imports every shipped model (754: NOLF GOTY
and AvP2 Primal Hunt), exports it and compares the two field by field. All
754 pass. Names, counts, flags, indices, triangles, weights, keyframe
times and strings, and the header data match exactly. Numbers match within
float precision (the worst cases are 6e-5 units in animation positions and
5e-5 in bone rotations, from Blender's bone matrices).

The exported file isn't byte-identical to the shipped one: it is rebuilt
from Blender's data, so floats differ in their last bits, and the original
order of each vertex's weights isn't kept (they're written by node).

## Textures

An imported DTX becomes a packed image with its header settings as custom
properties (`lt_bpp`, `lt_user_flags`, `lt_command`, `lt_texture_group`,
`lt_mipmaps_to_use`, `lt_mip_count`, `lt_priority`, `lt_detail_*`, and
sections such as `LIGHTDEFS`). Exporting writes those settings back.
*Format* picks 32-bit (lossless) or DXT1/3/5. The DXT encoder is a simple
one, not the original tools', so re-encoding a shipped DXT texture changes
its bytes slightly (2-3 levels per channel on average). Images must be
powers of two.

Cube maps import as their first face, and export as a flat texture.

## Not yet

- Shogo/Blood 2-era `.abc` and the `.ltb` models of NOLF2, Contract
  J.A.C.K. and Tron 2.0 (each should be its own module beside
  `lithtech/abc.py`).
- World files (`.dat`).
- Reading the games' attribute files to pick skins exactly.
