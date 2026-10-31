#!/bin/bash
# Start each GUI tool of a distribution on a virtual display and see that it
# comes up: still running after a few seconds, with a window. Run inside the
# build container (build.sh --shell), from the repository root:
#
#   legacy/jupiter/build/smoke.sh out/dist/lithtools-jupiter [tool.exe ...]
#
# For each tool it prints the windows it opened (xdotool) and saves a
# screenshot to out/smoke/<tool>.png. A tool that has exited by then is
# reported with Wine's last lines of output (out/smoke/<tool>.log).
set -u
dist="$1"; shift
tools=("$@")
[ ${#tools[@]} -gt 0 ] || tools=(dedit.exe ModelEdit.exe FxED.exe winpacker.exe
    RenderStylesEditor.exe ButeEdit.exe WaveEdit.exe WinLTC.exe DtxView.exe
    ltaview.exe)
mkdir -p out/smoke
export DISPLAY=:99
Xvfb :99 -screen 0 1280x1024x24 >/dev/null 2>&1 &
xvfb=$!
sleep 2
for t in "${tools[@]}"; do
    name="${t%.exe}"
    ( cd "$dist" && exec wine "$t" ) >"out/smoke/$name.log" 2>&1 &
    pid=$!
    sleep "${SMOKE_WAIT:-12}"
    wins=$(xdotool search --onlyvisible --name '.' 2>/dev/null |
           while read -r w; do xdotool getwindowname "$w"; done | sort -u |
           grep -v '^$' | tr '\n' '|')
    import -window root "out/smoke/$name.png" 2>/dev/null
    if kill -0 "$pid" 2>/dev/null; then
        echo "ok      $t   windows: ${wins:-none}"
    else
        echo "EXITED  $t   windows: ${wins:-none}"
        tail -5 "out/smoke/$name.log" | tr -d '\r' | sed 's/^/        /'
    fi
    wineserver -k >/dev/null 2>&1
    wait "$pid" 2>/dev/null
    sleep 1
done
kill "$xvfb" 2>/dev/null
