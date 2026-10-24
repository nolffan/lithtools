#!/bin/sh
# Packs a Linux build: lithrez and Rez Studio as an AppImage (Qt bundled by
# linuxdeploy), in <out>/<name>.tar.gz unpacking to <name>/.
#   linux.sh <build dir> <out dir> <name>
# Needs QMAKE (or qmake6/qmake on PATH) for the Qt plugin, and network to
# fetch linuxdeploy the first time (pinned below, cached in $TOOLS).
set -eu
mkdir -p "$2"
build=$(cd "$1" && pwd) out=$(cd "$2" && pwd) name=$3
here=$(cd "$(dirname "$0")" && pwd)
TOOLS=${TOOLS:-$build/packaging-tools}
mkdir -p "$TOOLS"
TOOLS=$(cd "$TOOLS" && pwd)
LD_VER=1-alpha-20250213-2
QT_VER=1-alpha-20250213-1
mkdir -p "$TOOLS" "$out"
get() {   # <url> <file>
    [ -s "$TOOLS/$2" ] || curl -fsSL -o "$TOOLS/$2" "$1"
    chmod +x "$TOOLS/$2"
}
get "https://github.com/linuxdeploy/linuxdeploy/releases/download/$LD_VER/linuxdeploy-x86_64.AppImage" linuxdeploy
get "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/$QT_VER/linuxdeploy-plugin-qt-x86_64.AppImage" linuxdeploy-plugin-qt
export APPIMAGE_EXTRACT_AND_RUN=1 NO_STRIP=1
: "${QMAKE:=$(command -v qmake6 || command -v qmake)}"
export QMAKE PATH="$TOOLS:$PATH"
# X11 is always deployed; add Wayland (when Qt has it) and offscreen.
plugdir=$("$QMAKE" -query QT_INSTALL_PLUGINS)/platforms
extra=libqoffscreen.so
for p in libqwayland-generic.so libqwayland-egl.so; do
    [ -f "$plugdir/$p" ] && extra="$extra;$p"
done
export EXTRA_PLATFORM_PLUGINS="$extra"

stage=$(mktemp -d)
app="$stage/AppDir"
mkdir -p "$app/usr/share/applications" "$app/usr/share/icons/hicolor/256x256/apps"
cp "$here/rezstudio.desktop" "$app/usr/share/applications/"
cp "$here/rezstudio.png" "$app/usr/share/icons/hicolor/256x256/apps/rezstudio.png"
(cd "$stage" && OUTPUT=RezStudio-x86_64.AppImage "$TOOLS/linuxdeploy" --appdir "$app" \
    --executable "$build/gui/rezstudio" \
    --desktop-file "$app/usr/share/applications/rezstudio.desktop" \
    --icon-file "$app/usr/share/icons/hicolor/256x256/apps/rezstudio.png" \
    --plugin qt --output appimage >/dev/null)

mkdir -p "$stage/$name"
cp "$build/lithrez" "$stage/$name/"
cp "$stage/RezStudio-x86_64.AppImage" "$stage/$name/"
cp "$here/../README.md" "$stage/$name/README.md"
tar -C "$stage" -czf "$out/$name.tar.gz" "$name"
rm -rf "$stage"
echo "$out/$name.tar.gz"
