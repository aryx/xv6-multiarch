#!/bin/bash
# ARCHIVED - this is a record of how this repo was built, not a
# maintained script. It reads clones from ~/work/xv6/ and does
# "rm -rf" on its output directory. Do NOT point it at ~/xv6.
# Superseded by build-multiarch.sh: 9 ports, before the last 4.
# See scripts/README.md.
#
set -euo pipefail

ORIG=/Users/pad/work/xv6
SCRATCH=/private/tmp/claude-501/-Users-pad-work-xv6/f59ed6d4-2c3f-41bb-8f66-482ed9587a2c/scratchpad
B=$SCRATCH/build3
FINAL=/Users/pad/work/xv6/xv6-multiarch
rm -rf "$B"; mkdir -p "$B"; cd "$B"

GA=(-c user.name="Claude Code" -c user.email="noreply@anthropic.com")
TMP=$(mktemp -d)

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

# Re-parent a fork onto its true upstream ancestor using pure plumbing.
#
# Only the fork's OWN commits (child + descendants) are recreated. Upstream
# commit objects are never read through fast-export/fast-import, so nothing
# upstream can be normalised -- in particular PGP-signed MIT commits keep
# their signatures and therefore their original hashes. That is what lets all
# nine architectures share one copy of the common xv6 history.
graft_plumbing () {
  local dir=$1 child=$2 origparent=$3 trunkpath=$4 trunkcommit=$5
  ( cd "$dir"
    git remote add trunksrc "$trunkpath" 2>/dev/null || true
    git fetch trunksrc --quiet
    local branch; branch=$(git rev-parse --abbrev-ref HEAD)
    local mapfile="$TMP/map.$$"; : > "$mapfile"

    lookup () { local k=$1; local v; v=$(grep "^$k " "$mapfile" 2>/dev/null | cut -d' ' -f2); echo "${v:-$k}"; }

    local c
    for c in $(git rev-list --reverse --topo-order "${child}^..${branch}"); do
      local tree; tree=$(git rev-parse "$c^{tree}")
      git log -1 --format=%B "$c" > "$TMP/msg"
      local pargs=()
      if [ "$c" = "$child" ]; then
        pargs=(-p "$origparent" -p "$trunkcommit")
      else
        local p
        for p in $(git rev-list --parents -n1 "$c" | cut -d' ' -f2-); do
          pargs+=(-p "$(lookup "$p")")
        done
      fi
      local new
      new=$(GIT_AUTHOR_NAME="$(git log -1 --format=%an "$c")" \
            GIT_AUTHOR_EMAIL="$(git log -1 --format=%ae "$c")" \
            GIT_AUTHOR_DATE="$(git log -1 --format=%aI "$c")" \
            GIT_COMMITTER_NAME="$(git log -1 --format=%cn "$c")" \
            GIT_COMMITTER_EMAIL="$(git log -1 --format=%ce "$c")" \
            GIT_COMMITTER_DATE="$(git log -1 --format=%cI "$c")" \
            git commit-tree "$tree" "${pargs[@]}" < "$TMP/msg")
      echo "$c $new" >> "$mapfile"
    done

    local newtip; newtip=$(lookup "$(git rev-parse "$branch")")
    git update-ref "refs/heads/$branch" "$newtip"
    git checkout -q -f "$branch"
    git reset -q --hard "$newtip"
    git remote remove trunksrc
  )
}

move_into_arch () {
  local dir=$1 name=$2
  ( cd "$dir"
    mkdir -p "arch/$name"
    git ls-tree --name-only HEAD | while read -r e; do git mv "$e" "arch/$name/"; done
    git "${GA[@]}" commit -q -m "Move $name tree into arch/$name/

Staging commit so this architecture can coexist with the others in one tree.
Nothing below this commit is rewritten -- the history underneath keeps its
original commit hashes, so all architectures share a single copy of the
common xv6 lineage instead of one copy each."
  )
}

echo "### grafts (same fork points as before, now via plumbing)"
graft_plumbing ./rpi1 9f5d9ac2beb268d4adc1f83c215c19e1ec828249 5be933e6c201b4e6ad199de203383b6e33849b58 "$B/x86" ff2783442ea2801a4bf6c76f198f36a6e985e7dd
RPI1_TIP=$(cd rpi1 && git rev-parse HEAD)
echo "  rpi1 tip: $RPI1_TIP"
graft_plumbing ./rpi2       50a6ec819d2fb2cb27e7f676dc764c0fb4c304e8 e220f2a41301bf8762b367826ca0d3dda10411ce "$B/rpi1" "$RPI1_TIP"
graft_plumbing ./armv6-rpi  204d40a510dfd86ed2813d750541b55250121269 7a6dee4904c89e89c4c1230f37675f2ac7dd10b7 "$B/rpi1" "$RPI1_TIP"
graft_plumbing ./armv7-rpi  ab5bfce1b1f38d1693a36b419f1f3cac5fdc7423 3d61d14f47ac006b52be86a15e6e14d418b745de "$B/x86" ff2783442ea2801a4bf6c76f198f36a6e985e7dd
graft_plumbing ./rv32       2c829c083faad14f3c56f4ce623048201486efdf 3eafa3c25c06eb224c1e268a7af327a96338b514 "$B/riscv" 050a69610afee9884bc3df27215d0d5534743975
graft_plumbing ./d1         ecb94ece27e5446798ac636b1a103a43d8d0058e be2e2180b15f2c1a8d0cd0db99fea2aac6ee2083 "$B/riscv" a1da53a5a12e21b44a2c79d962a437fa2107627c

echo "### amd64: no graft -- a different tip on the same MIT lineage"
( cd amd64 && git checkout -q -b amd64 0f90388c893d1924e89e2e4d2187eda0004e9d73 )

echo "### one move commit per arch"
for n in x86 riscv amd64 rpi1 rpi2 armv6-rpi armv7-rpi rv32 d1; do move_into_arch "./$n" "$n"; done

echo "### assembling $FINAL"
rm -rf "$FINAL"; mkdir -p "$FINAL"; cd "$FINAL"
git init --quiet
for n in riscv x86 amd64 rpi1 rpi2 armv6-rpi armv7-rpi rv32 d1; do
  git remote add "$n" "$B/$n"; git fetch "$n" --quiet
  git branch "arch/$n" "$(git -C "$B/$n" rev-parse HEAD)"
done

git read-tree --empty
PARENTS=()
for n in riscv x86 amd64 rpi1 rpi2 armv6-rpi armv7-rpi rv32 d1; do
  tip=$(git rev-parse "refs/heads/arch/$n")
  git read-tree --prefix="arch/$n/" "$tip:arch/$n"
  PARENTS+=(-p "$tip")
done
TREE=$(git write-tree)
COMMIT=$(git "${GA[@]}" commit-tree "$TREE" "${PARENTS[@]}" -m "Unify nine xv6 architectures under arch/

Octopus merge joining every architecture lineage into one tree. Each parent
keeps the original commit hashes of the shared MIT xv6 history, so the common
2006-2019 lineage is stored once and blames identically from every arch.

First parent is arch/riscv, whose history spans the whole MIT lineage
(x86 era -> x86-64 experiment -> RISC-V), so 'git log --first-parent' reads
as the upstream story.")
git checkout -q -b main "$COMMIT"
git reset -q --hard "$COMMIT"

for n in riscv x86 amd64 rpi1 rpi2 armv6-rpi armv7-rpi rv32 d1; do git remote remove "$n"; done
git gc --quiet 2>/dev/null || true
rm -rf "$TMP"
echo "### DONE"
