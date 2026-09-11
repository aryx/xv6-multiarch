#!/bin/bash
# claude: pairwise diff-line-count matrix across every fork that has a
# given file, sorted closest-first - the actual tool behind every
# mkfs.c/usertests.c factorization cluster found this session (see
# plan_factorization.md's own pairwise-diff tables, e.g. the "18 lines
# apart" arm64/arm64-pi4/loongarch usertests.c finding and the "48 lines"
# amd64/i386 one). Run repeatedly by hand as a bash one-liner before this
# existed; saved here so the next factorization pass doesn't retype it.
#
# Usage (from anywhere - cd's to the repo root itself):
#   scripts/pairwise_diff.sh tests/usertests.c
#   scripts/pairwise_diff.sh kernel/fs.h
#   scripts/pairwise_diff.sh tools/mkfs.c
#
# Compares forks/<name>/<relpath> across every fork that HAS that file
# (forks without it are silently skipped - useful on its own to see which
# forks a given path exists in at all), printing "<diff-lines> <a> <b>"
# one pair per line, ascending. A pair at or near 0 is a free/near-free
# win; the plan_factorization.md convention this session settled on is
# roughly: 0-20 lines is usually comment-only, 20-150 is a real but small
# reconciliation, 150+ needs a close read before assuming it's one cluster
# (see notes_arch_arm_pi3.txt's own Bug 22 for a case where a seemingly
# close pair - 128 lines - turned out to hide a real, separate kernel bug,
# not just style).
set -euo pipefail

if [ $# -ne 1 ]; then
  echo "usage: $0 <relpath-under-a-fork>  e.g. tests/usertests.c" >&2
  exit 1
fi
relpath="$1"

cd "$(dirname "$0")/.."

archs=()
for f in forks/*/"$relpath"; do
  [ -f "$f" ] || continue
  archs+=("$(basename "$(dirname "$(dirname "$f")")")")
done

if [ "${#archs[@]}" -lt 2 ]; then
  echo "fewer than two forks have $relpath - nothing to compare" >&2
  exit 1
fi

for a in "${archs[@]}"; do
  for b in "${archs[@]}"; do
    if [[ "$a" < "$b" ]]; then
      n=$(diff "forks/$a/$relpath" "forks/$b/$relpath" 2>/dev/null | grep -c '^[<>]' || true)
      echo "$n $a $b"
    fi
  done
done | sort -n
