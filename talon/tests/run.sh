#!/bin/bash
# Behaviour tests: run the shipped tools and ours on the same inputs and
# compare what they write (tests/README.md).
#
#   tests/run.sh [suite...]     every suite (tests/*.sh but this and lib.sh,
#                               less the pending ones, tests/pending.txt),
#                               or the ones named (processor-avp2, ...)
#
# Runs inside the build container, from the talon directory (`make test`
# does both). Needs `make reference` and `make`. Exits non-zero if any test
# fails; works in out/tests/.
set -uo pipefail
cd "$(dirname "$0")/.."
. tests/lib.sh

suites=("$@")
if [ ${#suites[@]} -eq 0 ]; then
    for f in tests/*.sh; do
        n=$(basename "$f" .sh)
        case "$n" in run|lib) continue ;; esac
        grep -qx "$n" tests/pending.txt 2>/dev/null && { log "(skipping $n: pending, tests/pending.txt)"; continue; }
        suites+=("$n")
    done
fi

start_wine
for s in "${suites[@]}"; do
    [ -f "tests/$s.sh" ] || { log "no suite $s (tests/$s.sh)"; exit 2; }
    log "== $s"
    . "tests/$s.sh"
    "suite_${s//-/_}"
done
if [ $FAILED -gt 0 ]; then
    log "$FAILED test(s) failed"
    exit 1
fi
log "all passed"
