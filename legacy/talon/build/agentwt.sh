#!/bin/bash
# A worktree of lithtools in which talon builds and scores on its own, for
# one agent of a round: its builds and links never touch another's.
#
#   build/agentwt.sh add <name> [start]   out/wt-<name>, branch talon/<name>
#   build/agentwt.sh rm <name>            remove it (the branch is kept)
#
# What the build needs and git doesn't track comes from this checkout:
# ext/toolchains and out/reference hard-linked (read only: never edit a
# file under them in a worktree), ext/tools copied (a checkout of its own,
# without its .git), and every target's build outputs and .build copied
# with their times, the worktree's sources dated before them, so that
# `make` rebuilds only what the worktree changes and `make progress` has
# every target to score. Each worktree has its own Wine prefix
# (legacy/talon/.wine), so builds in two of them don't collide.
# out/ is ignored by git; the worktree is under the repository so that
# hard links reach it.
set -eu
TALON="$(cd "$(dirname "$0")/.." && pwd)"
REPO="$(cd "$TALON/../.." && pwd)"
case "${1:-}" in
add)
    name="${2:?usage: agentwt.sh add <name> [start]}"
    wt="$REPO/out/wt-$name"
    git -C "$REPO" worktree add -q -b "talon/$name" "$wt" "${3:-HEAD}"
    rmdir "$wt/ext/tools" "$wt/ext/toolchains" "$wt/ext/rez" 2>/dev/null || true
    cp -al "$REPO/ext/toolchains" "$wt/ext/toolchains"
    rm -f "$wt/ext/toolchains/.git"
    cp -a "$REPO/ext/tools" "$wt/ext/tools"
    rm -f "$wt/ext/tools/.git"
    t="$wt/legacy/talon"
    # sources older than the build outputs copied next
    git -C "$wt" ls-files -z legacy/talon | (cd "$wt" && xargs -0 touch -h -d '2000-01-01')
    mkdir -p "$t/out"
    cp -al "$TALON/out/reference" "$t/out/reference"
    [ -d "$TALON/.build" ] && cp -a "$TALON/.build" "$t/.build"
    (cd "$TALON" && find src -type d -iname built -prune -print0) | while IFS= read -r -d '' d; do
        mkdir -p "$t/$(dirname "$d")"
        cp -a "$TALON/$d" "$t/$d"
    done
    echo "worktree: $wt (branch talon/$name)"
    echo "build:    make -C $t; score: make -C $t progress"
    ;;
rm)
    name="${2:?usage: agentwt.sh rm <name>}"
    git -C "$REPO" worktree remove --force "$REPO/out/wt-$name"
    git -C "$REPO" worktree prune
    ;;
*)  echo "usage: $0 add <name> [start] | rm <name>" >&2; exit 2 ;;
esac
