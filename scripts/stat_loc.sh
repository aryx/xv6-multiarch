#!/bin/bash
# claude: LOC snapshot for the kernel/<category>/<arch>/ reorg -
# kernel/ vs forks/ (real content, excluding symlinks, the vendored
# USPi USB stack, and anything untracked like build-generated blobs
# - loongarch's own kernel/ramdisk.h, an `xxd -i fs.img` dump, is the
# one that bit this the first time by hand). Uses `git ls-files` so
# it only ever counts tracked source, never a stray local build
# artifact.
#
# Usage: scripts/stat_loc.sh (from anywhere - cd's to the repo root)
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"

count() {
  # $1: pathspec prefix
  git ls-files "$1" \
    | grep -E '\.(c|h|S)$' \
    | grep -v '/uspi' \
    | while read -r f; do [ -L "$f" ] || echo "$f"; done \
    | xargs -r cat | wc -l
}

kernel_loc=$(count 'kernel')
forks_loc=$(count 'forks')
kernel_files=$(git ls-files kernel | grep -E '\.(c|h|S)$' | grep -v '/uspi' | while read -r f; do [ -L "$f" ] || echo "$f"; done | wc -l)
forks_files=$(git ls-files forks | grep -E '\.(c|h|S)$' | grep -v '/uspi' | while read -r f; do [ -L "$f" ] || echo "$f"; done | wc -l)

echo "kernel/: $kernel_loc LOC across $kernel_files files"
echo "forks/ (excl. vendored USPi): $forks_loc LOC across $forks_files files"
echo "total:  $((kernel_loc + forks_loc)) LOC"
