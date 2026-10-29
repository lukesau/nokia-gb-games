#!/usr/bin/env python3
"""Compare the port's Munkiki's Castles state log with the JAR's.

Usage: munkiki_compare.py PORT_LOG JAR_LOG

Both are the lines "T n", "K ...", "D ...", "C ...", "BZ ...", "BD ...",
"BA ...", "CL ...", "CK ..." and "S ..." the port's munkiki_replay and the
JAR's harness write after each tick, with the JAR's field names. Every
field the port writes must equal the JAR's; the JAR's extra fields are
ignored. Reports the first tick that differs, and what.
"""
import sys


def parse(path):
    ticks, cur = [], None
    for line in open(path):
        line = line.rstrip("\n")
        if line.startswith("T "):
            cur = {"T": int(line.split()[1])}
            ticks.append(cur)
        elif cur is None:
            continue
        elif line.startswith(("K ", "D ", "C ")):
            key = line[0]
            fields = {}
            for item in line[2:].split():
                if "=" in item:
                    k, v = item.split("=", 1)
                    fields[k] = v
            if line.strip() == "D null":
                fields = None
            cur[key] = fields
        elif line.startswith(("BZ", "BD", "BA", "CL", "CK", "S")):
            key = line.split(" ", 1)[0]
            cur[key] = line[len(key):].split()
    return ticks


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    ours, theirs = parse(sys.argv[1]), parse(sys.argv[2])
    n = min(len(ours), len(theirs))
    for i in range(n):
        a, b = ours[i], theirs[i]
        for key in ("K", "D", "C"):
            fa, fb = a.get(key), b.get(key)
            if fa is None and fb is None:
                continue
            if fa is None or fb is None:
                print(f"tick {a['T']}: {key} is {'null' if fa is None else 'set'} here, {'null' if fb is None else 'set'} in the JAR")
                sys.exit(1)
            for k, v in fa.items():
                if k in fb and fb[k] != v:
                    print(f"tick {a['T']}: {key}.{k} is {v} here, {fb[k]} in the JAR")
                    sys.exit(1)
        for key in ("BZ", "BD", "BA", "S"):
            if key in a and key in b and a[key] != b[key]:
                print(f"tick {a['T']}: {key} differs:\n  here {' '.join(a[key])}\n  JAR  {' '.join(b[key])}")
                sys.exit(1)
        for key in ("CL", "CK"):
            if key in a and key in b and a[key] != b[key]:
                sa, sb = a[key][0], b[key][0]
                width = 2 if key == "CL" else 4
                for c in range(0, min(len(sa), len(sb)), width):
                    if sa[c:c + width] != sb[c:c + width]:
                        cell = c // width
                        print(f"tick {a['T']}: {key} differs at cell {cell} (x {cell & 15}, y {cell >> 8}, z {(cell >> 4) & 15}): "
                              f"{sa[c:c + width]} here, {sb[c:c + width]} in the JAR")
                        sys.exit(1)
                print(f"tick {a['T']}: {key} differs in length")
                sys.exit(1)
    print(f"ok   {n} ticks agree" + ("" if len(ours) == len(theirs) else f" ({len(ours)} here, {len(theirs)} in the JAR)"))


if __name__ == "__main__":
    main()
