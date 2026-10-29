# Shared by the test suites (sourced; run inside the build container, from
# the talon directory, as tests/run.sh does).
#
# Every suite runs the shipped tool and ours on the same inputs, in the same
# conditions, and compares what they write: the shipped tool is the
# reference, so no expected output is stored here and nothing depends on the
# host's Wine behaving like another's.

TALON=${TALON:-$PWD}
REFERENCE="$TALON/out/reference"
WORK="$TALON/out/tests"
# outside out/tests: its dosdevices/z: is the whole filesystem, which must
# not be in what CI keeps of a failed run
export WINEPREFIX="$TALON/out/tests-wine" WINEDEBUG=-all
TEST_DISPLAY=:97
FAILED=0

log() { printf '%s\n' "$*"; }
pass() { log "  PASS  $*"; }
fail() { log "  FAIL  $*"; FAILED=$((FAILED + 1)); }

need() {   # need <file> <what to run to make it>
    [ -e "$1" ] || { log "missing $1: run $2 first"; exit 2; }
}

# One Wine prefix and one virtual display for the whole run.
start_wine() {
    mkdir -p "$WORK"
    [ -d "$WINEPREFIX/drive_c" ] || wineboot -i >/dev/null 2>&1
    Xvfb "$TEST_DISPLAY" -screen 0 1024x768x24 >/dev/null 2>&1 &
    XVFB_PID=$!
    trap stop_wine EXIT
    sleep 1
}
stop_wine() {
    wineserver -k >/dev/null 2>&1 || true
    [ -n "${XVFB_PID:-}" ] && kill "$XVFB_PID" 2>/dev/null || true
}

# run_tool <timeout s> <working dir> <exe> [args...]: run a Windows tool in
# <working dir> until it exits by itself; 124 if it had to be killed.
run_tool() {
    local limit=$1 dir=$2 rc=0
    shift 2
    (cd "$dir" && DISPLAY=$TEST_DISPLAY timeout "$limit" wine "$@" >wine.log 2>&1) || rc=$?
    wineserver -k >/dev/null 2>&1 || true
    return $rc
}

# winpath <path>: the Z: path Wine gives a Unix path
winpath() { printf 'Z:%s' "$(realpath -m "$1" | tr / '\\')"; }
