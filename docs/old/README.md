# MIT's own development notes for the original x86 xv6

Historical upstream material, kept for reference. **None of it describes
this repository**, and much of it no longer describes xv6 either.

| file | what it is |
|---|---|
| `BUGS` | MIT's own known-issues list for the x86 xv6 - formatting TODOs, an `sh` `cd` limitation, and similar |
| `Notes` | development memos: Bochs 2.2.6 `./configure` invocations, ELF section-alignment constraints in `bootmain.c` |
| `TRICKS` | "subtle things that might not be commented as well as they should be" - some entries dated 2009 and explicitly marked no longer relevant |

These were duplicated across several forks: `BUGS` in three (`i386`,
`amd64`, `mips`), `Notes` in three, `TRICKS` in four (those plus
`amd64-jserv`). All copies were byte-identical, so one copy of each is
kept here and the rest were dropped.

`amd64-jserv`'s own `Notes` genuinely differs from MIT's and is that
port's own working notes, so it lives in `docs/forks/amd64-jserv/`
instead.

For notes that DO describe this repository, see `docs/claude_notes/`.
