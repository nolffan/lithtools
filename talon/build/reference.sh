#!/bin/bash
# Extract the shipped tool binaries into out/reference/, the ground truth the
# scoreboard and the reconstruction tools compare against. Nothing in the
# build reads it, and nothing in it is ever run.
#
#   build/reference.sh
#
# Inputs, each checked before use (override the paths in the environment):
#   AVP2_PH_BIN  the Aliens versus Predator 2: Primal Hunt (Europe) disc image,
#                redump.org disc 35760 (archive.org Redump.org35760): its .bin,
#                SHA-1 b47b0750...747d. Its Tools folder has the AvP2 Editing
#                Tools v1.0 (2001-12-14) and Updates 1 and 2 (2002-01-31,
#                2002-05-06), InstallShield 6 self-extractors.
#   NOLF_TOOLS   NOLFTools001.exe from the NOLF Game of the Year disc 2 (the
#                rez/ release nolf-usa-goty), SHA-256 b2a13a39...71d1: the NOLF
#                Tools 001 (January 2001), a zip self-extractor.
#
# Writes out/reference/<release>/ for avp2tools, avp2tools_update,
# avp2tools_update2 and nolftools001, each the release's files as its
# installer lays them out, and SHA256SUMS over all of them.
set -euo pipefail
cd "$(dirname "$0")/.."
TALON="$PWD"
AVP2_PH_BIN="${AVP2_PH_BIN:-$TALON/../extracted/avp2-ph-eu/Aliens Versus Predator 2 - Primal Hunt (Europe).bin}"
NOLF_TOOLS="${NOLF_TOOLS:-$TALON/../extracted/nolf-tools-001/iso/Game/NOLFTools001.exe}"
PH_SHA1=b47b07507ad6a511958b8544498d4b9399c2747d
NOLF_SHA256=b2a13a39a12642b8de66b541dea999c5315c6764f0b6897bcaf7a01e832071d1
REZ="$TALON/rez/tools"
OUT="$TALON/out/reference"
TMP="$TALON/out/reference.tmp"

check() {   # check <file> <sha1|sha256> <digest>
    [ -f "$1" ] || { echo "error: missing $1 (see build/reference.sh)" >&2; exit 1; }
    local got; got=$("$2sum" "$1" | cut -d' ' -f1)
    [ "$got" = "$3" ] || { echo "error: $1: $2 $got, expected $3" >&2; exit 1; }
}
check "$AVP2_PH_BIN" sha1 "$PH_SHA1"
check "$NOLF_TOOLS" sha256 "$NOLF_SHA256"

make -s -C "$REZ/unshield" >/dev/null
rm -rf "$TMP"; mkdir -p "$TMP"

# The Primal Hunt disc: one Mode 1 (XA Form 1) data track of raw sectors; the
# ISO is each sector's 2048 bytes of user data.
python3 -I - "$AVP2_PH_BIN" "$TMP/ph.iso" <<'EOF'
import sys
src, dst = sys.argv[1:3]
with open(src, 'rb') as f, open(dst, 'wb') as o:
    while True:
        s = f.read(2352)
        if len(s) < 2352:
            break
        o.write(s[16:2064] if s[15] == 1 else s[24:2072])
EOF
python3 -I "$REZ/isoextract.py" x "$TMP/ph.iso" "$TMP/ph" Tools >/dev/null
for n in avp2tools avp2tools_update avp2tools_update2; do
    python3 -I "$REZ/sfxcab.py" "$TMP/ph/Tools/$n.exe" "$TMP/sfx-$n" >/dev/null
    hdr=$(find "$TMP/sfx-$n" -iname data1.hdr | head -1)
    "$REZ/unshield/unshield" -d "$TMP/x-$n" x "$hdr" >/dev/null
    # the setup's own support groups (_Engine_*, _Support_*) are not the release
    mkdir -p "$TMP/$n"
    for g in "$TMP/x-$n"/*; do
        case "$(basename "$g")" in _*) ;; *) cp -a "$g"/. "$TMP/$n"/ ;; esac
    done
done

# NOLF Tools 001: a zip self-extractor around an InstallShield 5 setup whose
# files are stored plain in the zip.
python3 -I -c "import zipfile,sys; zipfile.ZipFile(sys.argv[1]).extractall(sys.argv[2])" \
    "$NOLF_TOOLS" "$TMP/nolftools001"

rm -rf "$OUT"; mkdir -p "$OUT"
for n in avp2tools avp2tools_update avp2tools_update2 nolftools001; do
    mv "$TMP/$n" "$OUT/$n"
done
rm -rf "$TMP"
(cd "$OUT" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum) >"$OUT/SHA256SUMS"
echo "out/reference: $(wc -l <"$OUT/SHA256SUMS") files"

# Each target's function table (tools/enumfuncs.py), as progress/targets.py
# names them: out/reference/<stem>-functions.csv
python3 -I - <<'PY'
import os, subprocess, sys
sys.path.insert(0, 'progress')
import targets
for t in targets.TARGETS:
    if t.get('kind') != 'image':
        continue
    csv = os.path.join(targets.REFERENCE, t['stem'] + '-functions.csv')
    subprocess.run([sys.executable, '-I', 'tools/enumfuncs.py', t['shipped'], '--csv', csv],
                   check=True, stdout=subprocess.DEVNULL)
    print('wrote', csv)
PY
