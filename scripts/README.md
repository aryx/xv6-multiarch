# scripts/

Archived record of how this repository was assembled, kept so the work is
traceable rather than folklore. **Nothing here is maintained or meant to be
run today** — the repo has moved on considerably since (build system, CI,
per-port fixes), and these scripts would rebuild it from scratch at its old
path, discarding all of that.

They read the twelve upstream clones from `~/work/xv6/` and each does
`rm -rf` on its output directory. Do not repoint one at `~/xv6`.

The reasoning behind every fork point and technique below is in
`docs/provenance.md`; this file only says which script is which.

## The one that built this repo

| file | what it did |
|---|---|
| `build-multiarch.sh` | Produced the current history: 13 ports, 2,050 commits, 2026-09-05. Grafts the seven fresh-`git init` ports onto their fork points using `git commit-tree` plumbing, adds one `arch/<name>/` staging commit per port, then unions all thirteen tips into an octopus merge built with `read-tree`/`commit-tree`. |

## Superseded attempts, kept for the lessons

Each failed in a way that shaped the final approach, and each lesson is
written up in `docs/provenance.md` under "Four pitfalls".

| file | why it was abandoned |
|---|---|
| `superseded/try1-subdirectory-filter.sh` | Used `git filter-repo --to-subdirectory-filter` over each port's whole history. That rewrites every commit, so the shared MIT lineage was duplicated once per arch — 9,932 commits instead of 1,795, with every old commit message appearing nine times in `git log`. |
| `superseded/try2-filter-repo-grafts.sh` | Right idea (one staging commit per port), but still used `filter-repo` to bake the grafts. `filter-repo` **strips GPG signatures**, which changes those commits' hashes and cascades through every descendant — silently duplicating ~335 riscv commits into `rv32` and ~173 into `d1`. Hence the switch to `git commit-tree`. |
| `superseded/try3-nine-arches.sh` | Correct approach, but predates the discovery of `x86_64`, `mips`, `aarch64` and `loongarch`. Superseded by `build-multiarch.sh` rather than wrong. |

## Analysis helpers

These are still useful — they produced the duplication figures that shape
`docs/claude_notes/factorization-plan.md`, and re-running them will show
progress once that phase starts. Both read `git ls-tree` output on stdin:

```sh
git ls-tree -r HEAD --format='%(objectname) %(path)' | python3 scripts/analyze-duplication.py
git ls-tree -r HEAD --format='%(objectname) %(path)' | python3 scripts/analyze-variants.py
```

| file | output |
|---|---|
| `analyze-duplication.py` | Per filename: how many ports have it, and how many distinct contents exist. Gave "930 copies, 659 distinct contents". |
| `analyze-variants.py` | For candidate files, the largest set of ports sharing byte-identical content. This is what revealed the two families — seven ports share one `echo.c`, four share another. |

Note both still assume the old `arch/<name>/` path prefix and will need
`arch` changed to `forks` to run against the current tree.
