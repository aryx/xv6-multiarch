#!/bin/bash
# ARCHIVED - this is a record of how this repo was built, not a
# maintained script. It reads clones from ~/work/xv6/ and does
# "rm -rf" on its output directory. Do NOT point it at ~/xv6.
# Superseded: filter-repo stripped GPG signatures.
# See scripts/README.md.
#
set -euo pipefail

ORIG=/Users/pad/work/xv6
SCRATCH=/private/tmp/claude-501/-Users-pad-work-xv6/f59ed6d4-2c3f-41bb-8f66-482ed9587a2c/scratchpad
B=$SCRATCH/build2
FINAL=/Users/pad/work/xv6/xv6-multiarch
rm -rf "$B"; mkdir -p "$B"; cd "$B"

GA=(-c user.name="Claude Code" -c user.email="noreply@anthropic.com")

echo "### copying pristine sources"
cp -R "$ORIG/xv6-x86"       ./x86
cp -R "$ORIG/xv6-riscv"     ./riscv
cp -R "$ORIG/xv6-riscv"     ./amd64
cp -R "$ORIG/xv6_rpi_port"  ./rpi1
cp -R "$ORIG/xv6_rpi2_port" ./rpi2
cp -R "$ORIG/xv6-armv6-rpi" ./armv6-rpi
cp -R "$ORIG/xv6-armv7-rpi" ./armv7-rpi
cp -R "$ORIG/xv6-rv32"      ./rv32
cp -R "$ORIG/xv6-d1"        ./d1

# graft a fork onto its true parent, then bake it in.
# NOTE: this rewrites ONLY the fork's own commits; ancestors keep original hashes.
graft_and_bake () {
  local dir=$1 child=$2 origparent=$3 trunkpath=$4 trunkcommit=$5
  ( cd "$dir"
    git remote add trunksrc "$trunkpath" 2>/dev/null || true
    git fetch trunksrc --quiet
    git replace --graft "$child" "$origparent" "$trunkcommit"
    git filter-repo --force --replace-refs delete-no-add >/dev/null
  )
}

# the whole point of build2: ONE move commit per arch, instead of rewriting
# every commit with --to-subdirectory-filter.
move_into_arch () {
  local dir=$1 name=$2
  ( cd "$dir"
    mkdir -p "arch/$name"
    git ls-tree --name-only HEAD | while read -r e; do git mv "$e" "arch/$name/"; done
    git "${GA[@]}" commit -q -m "Move $name tree into arch/$name/

Staging commit so this architecture can coexist with the others in one
tree. Nothing below this commit is rewritten: the history underneath keeps
its original commit hashes, which is what lets all architectures share one
copy of the common xv6 lineage instead of nine."
  )
}

echo "### grafts (unchanged from try1 -- same fork points, same evidence)"
graft_and_bake ./rpi1 9f5d9ac2beb268d4adc1f83c215c19e1ec828249 5be933e6c201b4e6ad199de203383b6e33849b58 "$B/x86" ff2783442ea2801a4bf6c76f198f36a6e985e7dd
RPI1_TIP=$(cd rpi1 && git rev-parse HEAD)
echo "  rpi1 baked tip: $RPI1_TIP"
graft_and_bake ./rpi2       50a6ec819d2fb2cb27e7f676dc764c0fb4c304e8 e220f2a41301bf8762b367826ca0d3dda10411ce "$B/rpi1" "$RPI1_TIP"
graft_and_bake ./armv6-rpi  204d40a510dfd86ed2813d750541b55250121269 7a6dee4904c89e89c4c1230f37675f2ac7dd10b7 "$B/rpi1" "$RPI1_TIP"
graft_and_bake ./armv7-rpi  ab5bfce1b1f38d1693a36b419f1f3cac5fdc7423 3d61d14f47ac006b52be86a15e6e14d418b745de "$B/x86" ff2783442ea2801a4bf6c76f198f36a6e985e7dd
graft_and_bake ./rv32       2c829c083faad14f3c56f4ce623048201486efdf 3eafa3c25c06eb224c1e268a7af327a96338b514 "$B/riscv" 050a69610afee9884bc3df27215d0d5534743975
graft_and_bake ./d1         ecb94ece27e5446798ac636b1a103a43d8d0058e be2e2180b15f2c1a8d0cd0db99fea2aac6ee2083 "$B/riscv" a1da53a5a12e21b44a2c79d962a437fa2107627c

echo "### amd64: no graft -- just a different tip on the same MIT lineage"
( cd amd64 && git checkout -q -b amd64 0f90388c893d1924e89e2e4d2187eda0004e9d73 )

echo "### one move commit per arch"
move_into_arch ./x86        x86
move_into_arch ./riscv      riscv
move_into_arch ./amd64      amd64
move_into_arch ./rpi1       rpi1
move_into_arch ./rpi2       rpi2
move_into_arch ./armv6-rpi  armv6-rpi
move_into_arch ./armv7-rpi  armv7-rpi
move_into_arch ./rv32       rv32
move_into_arch ./d1         d1

echo "### assembling $FINAL"
rm -rf "$FINAL"; mkdir -p "$FINAL"; cd "$FINAL"
git init --quiet
for n in riscv x86 amd64 rpi1 rpi2 armv6-rpi armv7-rpi rv32 d1; do
  git remote add "$n" "$B/$n"
  git fetch "$n" --quiet
done

# branch per arch, pointing at each move commit
git branch arch/riscv     "$(git -C "$B/riscv"      rev-parse HEAD)"
git branch arch/x86       "$(git -C "$B/x86"        rev-parse HEAD)"
git branch arch/amd64     "$(git -C "$B/amd64"      rev-parse HEAD)"
git branch arch/rpi1      "$(git -C "$B/rpi1"       rev-parse HEAD)"
git branch arch/rpi2      "$(git -C "$B/rpi2"       rev-parse HEAD)"
git branch arch/armv6-rpi "$(git -C "$B/armv6-rpi"  rev-parse HEAD)"
git branch arch/armv7-rpi "$(git -C "$B/armv7-rpi"  rev-parse HEAD)"
git branch arch/rv32      "$(git -C "$B/rv32"       rev-parse HEAD)"
git branch arch/d1        "$(git -C "$B/d1"         rev-parse HEAD)"

# union tree built with plumbing: git merge can't do this without rename/rename
# conflicts, because every branch renamed the same ancestor files differently.
git read-tree --empty
PARENTS=()
for n in riscv x86 amd64 rpi1 rpi2 armv6-rpi armv7-rpi rv32 d1; do
  tip=$(git rev-parse "refs/heads/arch/$n")
  git read-tree --prefix="arch/$n/" "$tip:arch/$n"
  PARENTS+=(-p "$tip")
done
TREE=$(git write-tree)
MSG="Unify nine xv6 architectures under arch/

Octopus merge joining every architecture lineage into one tree. Each parent
keeps the original commit hashes of the shared MIT xv6 history, so the common
2006-2019 lineage is stored once and blames identically from every arch.

First parent is arch/riscv, whose history spans the entire MIT lineage
(x86 era -> x86-64 experiment -> RISC-V), so 'git log --first-parent' reads
as the upstream story."
COMMIT=$(git "${GA[@]}" commit-tree "$TREE" "${PARENTS[@]}" -m "$MSG")
git checkout -q -b main "$COMMIT"
git reset --hard -q "$COMMIT"

for n in riscv x86 amd64 rpi1 rpi2 armv6-rpi armv7-rpi rv32 d1; do git remote remove "$n"; done
git gc --quiet --aggressive 2>/dev/null || git gc --quiet
echo "### DONE"
