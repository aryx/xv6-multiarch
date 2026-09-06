#!/bin/bash
# ARCHIVED - this is a record of how this repo was built, not a
# maintained script. It reads clones from ~/work/xv6/ and does
# "rm -rf" on its output directory. Do NOT point it at ~/xv6.
# Superseded: duplicated the shared history once per arch.
# See scripts/README.md.
#
set -euo pipefail

ORIG=/Users/pad/work/xv6
SCRATCH=/private/tmp/claude-501/-Users-pad-work-xv6/f59ed6d4-2c3f-41bb-8f66-482ed9587a2c/scratchpad
BUILD=$SCRATCH/build
rm -rf "$BUILD"
mkdir -p "$BUILD"
cd "$BUILD"

echo "### copying pristine sources into build area"
cp -R "$ORIG/xv6-x86"        ./x86
cp -R "$ORIG/xv6-riscv"      ./riscv
cp -R "$ORIG/xv6_rpi_port"   ./rpi1
cp -R "$ORIG/xv6_rpi2_port"  ./rpi2
cp -R "$ORIG/xv6-armv6-rpi"  ./armv6-rpi
cp -R "$ORIG/xv6-armv7-rpi"  ./armv7-rpi
cp -R "$ORIG/xv6-rv32"       ./rv32
cp -R "$ORIG/xv6-d1"         ./d1

graft_and_bake () {
  # $1=repo dir  $2=real-content commit  $3=its-original-parent  $4=trunk-remote-path  $5=trunk-parent-commit
  local dir=$1 child=$2 origparent=$3 trunkpath=$4 trunkcommit=$5
  ( cd "$dir"
    git remote add trunksrc "$trunkpath" 2>/dev/null || true
    git fetch trunksrc --quiet
    git replace --graft "$child" "$origparent" "$trunkcommit"
    git filter-repo --force --replace-refs delete-no-add
  )
}

echo "### rpi1: graft onto x86 @ ff27834 (2013-03-04)"
graft_and_bake ./rpi1 9f5d9ac2beb268d4adc1f83c215c19e1ec828249 5be933e6c201b4e6ad199de203383b6e33849b58 "$BUILD/x86" ff2783442ea2801a4bf6c76f198f36a6e985e7dd
RPI1_TIP=$(cd rpi1 && git log --format=%H -1)
echo "rpi1 flat tip (post-bake): $RPI1_TIP"

echo "### rpi2: graft onto rpi1 tip (post-bake, flat)"
graft_and_bake ./rpi2 50a6ec819d2fb2cb27e7f676dc764c0fb4c304e8 e220f2a41301bf8762b367826ca0d3dda10411ce "$BUILD/rpi1" "$RPI1_TIP"

echo "### armv6-rpi: graft onto rpi1 tip (post-bake, flat)"
graft_and_bake ./armv6-rpi 204d40a510dfd86ed2813d750541b55250121269 7a6dee4904c89e89c4c1230f37675f2ac7dd10b7 "$BUILD/rpi1" "$RPI1_TIP"

echo "### armv7-rpi: graft onto x86 @ ff27834 (same base as rpi1, independent sibling)"
graft_and_bake ./armv7-rpi ab5bfce1b1f38d1693a36b419f1f3cac5fdc7423 3d61d14f47ac006b52be86a15e6e14d418b745de "$BUILD/x86" ff2783442ea2801a4bf6c76f198f36a6e985e7dd

echo "### rv32: graft onto riscv @ 050a696 (2020-07-23)"
graft_and_bake ./rv32 2c829c083faad14f3c56f4ce623048201486efdf 3eafa3c25c06eb224c1e268a7af327a96338b514 "$BUILD/riscv" 050a69610afee9884bc3df27215d0d5534743975

echo "### d1: graft onto riscv @ a1da53a (2021-09-01)"
graft_and_bake ./d1 ecb94ece27e5446798ac636b1a103a43d8d0058e be2e2180b15f2c1a8d0cd0db99fea2aac6ee2083 "$BUILD/riscv" a1da53a5a12e21b44a2c79d962a437fa2107627c

echo "### rpi1: now move to arch/rpi1 subdirectory (done last so rpi2/armv6-rpi could graft onto its flat tip)"
( cd rpi1 && git filter-repo --force --to-subdirectory-filter arch/rpi1 )

echo "### rpi2, armv6-rpi, armv7-rpi, rv32, d1: move to their arch subdirectories"
( cd rpi2 && git filter-repo --force --to-subdirectory-filter arch/rpi2 )
( cd armv6-rpi && git filter-repo --force --to-subdirectory-filter arch/armv6-rpi )
( cd armv7-rpi && git filter-repo --force --to-subdirectory-filter arch/armv7-rpi )
( cd rv32 && git filter-repo --force --to-subdirectory-filter arch/rv32 )
( cd d1 && git filter-repo --force --to-subdirectory-filter arch/d1 )

echo "### x86: full history, no graft, move to arch/x86"
( cd x86 && git filter-repo --force --to-subdirectory-filter arch/x86 )

echo "### riscv: full history -> arch/riscv branch"
cp -R riscv riscv-full
( cd riscv-full && git filter-repo --force --to-subdirectory-filter arch/riscv )

echo "### amd64: riscv history truncated at 0f90388 (last pre-riscv amd64 state) -> arch/amd64 branch"
( cd riscv
  git branch amd64-tip 0f90388c893d1924e89e2e4d2187eda0004e9d73
  git filter-repo --force --refs amd64-tip --to-subdirectory-filter arch/amd64
)

echo "### DONE building processed per-arch repos in $BUILD"
