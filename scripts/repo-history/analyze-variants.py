import sys, collections
by = collections.defaultdict(lambda: collections.defaultdict(list))
for line in sys.stdin:
    h, p = line.split(None, 1); p = p.strip(); parts = p.split("/")
    if len(parts) < 3 or parts[0] != "arch": continue
    base = parts[-1]
    if not base.endswith((".c", ".h", ".S")): continue
    by[base][h].append(parts[1])
targets = ["echo.c","ln.c","mkdir.c","rm.c","wc.c","umalloc.c","kill.c","stressfs.c",
           "forktest.c","grep.c","ls.c","sh.c","cat.c","init.c","zombie.c",
           "fcntl.h","stat.h","syscall.h","fs.h","user.h"]
print("%-13s %6s  %s" % ("file", "arches", "largest identical group"))
for t in targets:
    if t not in by: continue
    groups = sorted(by[t].values(), key=len, reverse=True)
    n = sum(len(g) for g in groups)
    print("%-13s %6d  %2d arches: %s" % (t, n, len(groups[0]), " ".join(sorted(groups[0]))))
