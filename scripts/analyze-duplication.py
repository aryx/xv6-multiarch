import sys, collections

by = collections.defaultdict(lambda: (set(), set()))
for line in sys.stdin:
    h, p = line.split(None, 1)
    p = p.strip()
    parts = p.split("/")
    if len(parts) < 3 or parts[0] != "arch":
        continue
    base = parts[-1]
    if not base.endswith((".c", ".h", ".S", ".s")):
        continue
    a, hs = by[base]
    a.add(parts[1])
    hs.add(h)

rows = [(len(a), len(hs), b) for b, (a, hs) in by.items() if len(a) >= 5]
rows.sort(key=lambda r: (r[1], -r[0]))
print("%-16s %6s %9s" % ("file", "arches", "variants"))
for na, nh, b in rows[:34]:
    print("%-16s %6d %9d" % (b, na, nh))

tot_files = sum(len(a) for a, hs in by.values())
tot_var = sum(len(hs) for a, hs in by.values())
print("\nacross all shared basenames: %d copies, %d distinct contents" % (tot_files, tot_var))
