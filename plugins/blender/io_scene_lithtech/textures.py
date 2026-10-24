"""DTX textures as Blender images, and finding a model's skins.

An imported image is packed into the .blend (Blender can't read a .dtx
itself) and keeps the DTX header's settings as custom properties (lt_bpp,
lt_user_flags, lt_iflags, lt_command, lt_texture_group, lt_mipmaps_to_use,
lt_priority, lt_detail_scale, lt_detail_angle, lt_sections) so exporting it
again writes the same settings.
"""

import base64
import json
import os

import bpy

from .lithtech import dtx

try:
    import numpy as np
except ImportError:  # Blender bundles numpy; this is for safety only.
    np = None


BPP_ITEMS = (
    ('32', "32-bit BGRA", "Uncompressed, lossless", dtx.BPP_32),
    ('DXT1', "DXT1", "S3TC, 1-bit alpha", dtx.BPP_S3TC_DXT1),
    ('DXT3', "DXT3", "S3TC, sharp 4-bit alpha", dtx.BPP_S3TC_DXT3),
    ('DXT5', "DXT5", "S3TC, smooth alpha", dtx.BPP_S3TC_DXT5),
)
BPP_BY_ID = {i[0]: i[3] for i in BPP_ITEMS}
ID_BY_BPP = {i[3]: i[0] for i in BPP_ITEMS}


def _rgba_to_pixels(w, h, rgba):
    flipped = dtx.flip_rows(rgba, w, h)
    if np is not None:
        return np.frombuffer(flipped, dtype=np.uint8).astype(np.float32) / 255.0
    return [b / 255.0 for b in flipped]


def _pixels_to_rgba(image):
    w, h = image.size
    if np is not None:
        px = np.empty(w * h * 4, dtype=np.float32)
        image.pixels.foreach_get(px)
        data = (np.clip(px, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8).tobytes()
    else:
        data = bytes(int(max(0.0, min(1.0, p)) * 255.0 + 0.5) for p in image.pixels[:])
    return w, h, dtx.flip_rows(data, w, h)


def load_image(path, name=None):
    """A DTX file as a packed Blender image."""
    t = dtx.read_file(path)
    w, h, rgba = t.decode(0)
    name = name or os.path.basename(path)
    has_alpha = rgba[3::4] != b"\xff" * (w * h)
    img = bpy.data.images.new(name, w, h, alpha=True)
    img.pixels.foreach_set(_rgba_to_pixels(w, h, rgba))
    # Skins often use alpha as a mask (or leave it 0), so it never makes the
    # material see-through on its own.
    img.alpha_mode = 'CHANNEL_PACKED' if has_alpha else 'NONE'
    img["lt_has_alpha"] = has_alpha
    img.pack()
    img.filepath_raw = ""
    img["lt_source"] = path
    img["lt_bpp"] = t.bpp
    img["lt_user_flags"] = t.user_flags
    img["lt_iflags"] = t.iflags
    img["lt_command"] = t.command
    img["lt_texture_group"] = t.texture_group
    img["lt_mipmaps_to_use"] = t.mipmaps_to_use
    img["lt_mip_count"] = t.mip_count
    img["lt_priority"] = t.priority
    img["lt_detail_scale"] = t.detail_scale
    img["lt_detail_angle"] = t.detail_angle
    # Sections other than the palette (texture lights, cube faces) as JSON.
    keep = [s for s in t.sections if s.type != dtx.SECTION_PALETTE32
            and s.type != dtx.SECTION_CUBEMAP]
    if keep:
        img["lt_sections"] = json.dumps(
            [{"type": s.type, "name": s.name,
              "data": base64.b64encode(s.data).decode("ascii")} for s in keep])
    return img


def save_image(image, path, bpp=None, mipmaps=None):
    """Write a Blender image as a DTX. bpp/mipmaps default to the image's
    imported settings, else 32-bit and a full chain."""
    w, h, rgba = _pixels_to_rgba(image)
    if bpp is None:
        bpp = image.get("lt_bpp", dtx.BPP_32)
        if bpp not in ID_BY_BPP:      # palettized etc.: no encoder, keep it lossless
            bpp = dtx.BPP_32
    if mipmaps is None:
        mipmaps = image.get("lt_mip_count") or None
    if mipmaps:
        mipmaps = max(1, min(int(mipmaps), dtx.max_mips_for_size(w), dtx.max_mips_for_size(h)))
    t = dtx.from_rgba(
        w, h, rgba, bpp=bpp, mipmaps=mipmaps,
        # A cube map's other faces aren't in the image, so it saves as flat.
        iflags=image.get("lt_iflags", dtx.DTX_SECTIONSFIXED) & dtx.DTX_FLAGSAVEMASK
        & ~dtx.DTX_CUBEMAP | dtx.DTX_SECTIONSFIXED,
        user_flags=image.get("lt_user_flags", 0),
        command=image.get("lt_command", ""),
        texture_group=image.get("lt_texture_group", 0),
        priority=image.get("lt_priority", 0),
        detail_scale=image.get("lt_detail_scale", 1.0),
        detail_angle=image.get("lt_detail_angle", 0))
    if "lt_mipmaps_to_use" in image:
        t.mipmaps_to_use = image["lt_mipmaps_to_use"]
    if "lt_sections" in image:
        for s in json.loads(image["lt_sections"]):
            t.sections.append(dtx.Section.make(s["type"], base64.b64decode(s["data"]), s["name"]))
        dtx.sync_counts(t)
    dtx.write_file(t, path)
    return t


# --------------------------------------------------------------------- #
#  Skins
# --------------------------------------------------------------------- #

def _listdir_nocase(d):
    try:
        return {e.lower(): os.path.join(d, e) for e in os.listdir(d)}
    except OSError:
        return {}


def _skin_dirs(model_path, extra_dirs):
    """Where a model's skins may be: the extra directories, the model's own,
    and the SKINS directory beside each MODELS directory above it (NOLF and
    AvP2 keep Foo/MODELS/x.abc with Foo/SKINS/x.dtx)."""
    dirs = [d for d in extra_dirs if d]
    d = os.path.dirname(os.path.abspath(model_path))
    dirs.append(d)
    sub = []
    while True:
        parent, base = os.path.split(d)
        if not base:
            break
        if base.lower() in ("models", "model"):
            for e in _listdir_nocase(parent).items():
                if e[0] in ("skins", "skin", "textures") and os.path.isdir(e[1]):
                    t = e[1]
                    for s in reversed(sub):
                        t = _listdir_nocase(t).get(s.lower(), t)
                    dirs.append(t)
                    dirs.append(e[1])
            break
        sub.append(base)
        d = parent
    return dirs


def skin_candidates(model_path, extra_dirs=()):
    """The DTX files that look like a model's skins, best first: <stem>.dtx,
    then <stem>_*.dtx and <stem>N.dtx sorted."""
    stem = os.path.splitext(os.path.basename(model_path))[0].lower()
    exact, others = [], []
    seen = set()
    for d in _skin_dirs(model_path, extra_dirs):
        for low, path in sorted(_listdir_nocase(d).items()):
            if not low.endswith(".dtx") or path in seen:
                continue
            s = low[:-4]
            if s == stem:
                exact.append(path)
                seen.add(path)
            elif s.startswith(stem) and (s[len(stem)] in "_-" or s[len(stem):].isdigit()):
                others.append(path)
                seen.add(path)
    return exact + others


def find_skin(model_path, slot, n_slots, extra_dirs=(), explicit=()):
    """A Blender image for skin slot `slot`, or None. explicit is a list of
    DTX paths by slot, from the import options."""
    path = None
    if slot < len(explicit) and explicit[slot]:
        path = explicit[slot]
    else:
        cands = skin_candidates(model_path, extra_dirs)
        if slot < len(cands):
            path = cands[slot]
    if not path:
        return None
    for img in bpy.data.images:
        if img.get("lt_source") == path:
            return img
    try:
        return load_image(path)
    except (OSError, ValueError) as e:
        print("LithTech: can't load skin %s: %s" % (path, e))
        return None


def make_material(name, image):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = next((n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'), None)
    if bsdf is not None:
        bsdf.inputs["Roughness"].default_value = 0.8
    if image is not None and bsdf is not None:
        tex = nt.nodes.new('ShaderNodeTexImage')
        tex.image = image
        tex.location = (bsdf.location.x - 320, bsdf.location.y)
        tex.interpolation = 'Linear'
        nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def material_image(mat):
    """The image a material's Base Color comes from, if any."""
    if mat is None or not mat.use_nodes:
        return None
    for n in mat.node_tree.nodes:
        if n.type == 'TEX_IMAGE' and n.image is not None:
            return n.image
    return None
