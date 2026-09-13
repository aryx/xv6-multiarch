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

list_files() {
  # $1: pathspec prefix - every tracked, non-symlink .c/.h/.S/.s file,
  # excluding the vendored USPi/CSUD USB stack.
  git ls-files "$1" \
    | grep -iE '\.(c|h|s)$' \
    | grep -v '/uspi' \
    | while read -r f; do [ -L "$f" ] || echo "$f"; done
}

kernel_loc=$(list_files 'kernel' | xargs -r cat | wc -l)
forks_loc=$(list_files 'forks' | xargs -r cat | wc -l)
kernel_files=$(list_files 'kernel' | wc -l)
forks_files=$(list_files 'forks' | wc -l)

echo "kernel/: $kernel_loc LOC across $kernel_files files"
echo "forks/ (excl. vendored USPi): $forks_loc LOC across $forks_files files"
echo "total:  $((kernel_loc + forks_loc)) LOC"
