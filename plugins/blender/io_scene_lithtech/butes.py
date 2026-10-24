"""The skins a game's attribute files give a model.

NOLF2, Contract J.A.C.K. and Tron 2.0 name each character's model and
skins in ATTRIBUTES/MODELBUTES.TXT:

    [Model0]
    Name        = "Player"
    ModelFile   = "chars\\models\\player_action.ltb"
    Skin0       = "chars\\skins\\CateAction.dtx"
    Skin1       = "chars\\skins\\CateCasualHead.dtx"

so a model inside an extracted game tree can find its skins by slot.
"""

import os
import re

_SECTION = re.compile(r"^\s*\[(.+?)\]")
_KEY = re.compile(r'^\s*([A-Za-z0-9_]+)\s*=\s*(?:"([^"]*)"|(\S+))')


def _find_nocase(base, rel):
    cur = base
    for part in rel.replace("\\", "/").split("/"):
        if not part:
            continue
        try:
            match = next((e for e in os.listdir(cur) if e.lower() == part.lower()), None)
        except OSError:
            return None
        if match is None:
            return None
        cur = os.path.join(cur, match)
    return cur


def game_root(model_path):
    """The directory above the model that holds ATTRIBUTES/MODELBUTES.TXT."""
    d = os.path.dirname(os.path.abspath(model_path))
    while True:
        if _find_nocase(d, "attributes/modelbutes.txt"):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            return None
        d = parent


def parse(path):
    """[(section, {key: value})] of a butes file (comments dropped)."""
    out = []
    cur = None
    with open(path, encoding="latin-1") as f:
        for line in f:
            line = line.split("//", 1)[0]
            m = _SECTION.match(line)
            if m:
                cur = {}
                out.append((m.group(1), cur))
                continue
            if cur is None:
                continue
            m = _KEY.match(line)
            if m:
                cur.setdefault(m.group(1), m.group(2) if m.group(2) is not None else m.group(3))
    return out


def model_skins(model_path):
    """(section name, [skin paths by slot], [render style paths]) for the
    first model in MODELBUTES.TXT whose ModelFile is this file, or None."""
    root = game_root(model_path)
    if root is None:
        return None
    rel = os.path.relpath(os.path.abspath(model_path), root).replace(os.sep, "\\").lower()
    for name, keys in parse(_find_nocase(root, "attributes/modelbutes.txt")):
        mf = keys.get("ModelFile") or keys.get("Filename")
        if not mf or mf.replace("/", "\\").lower() != rel:
            continue
        skins = []
        i = 0
        while "Skin%d" % i in keys:
            p = _find_nocase(root, keys["Skin%d" % i])
            skins.append(p or "")
            i += 1
        styles = []
        i = 0
        while "RenderStyle%d" % i in keys:
            styles.append(keys["RenderStyle%d" % i])
            i += 1
        return keys.get("Name", name), skins, styles
    return None
