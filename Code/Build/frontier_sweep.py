"""Find [FRONTIER] declarations that have no body - the ones /FORCE turns into RVA 0.

    python Code\\Build\\frontier_sweep.py [path\\to\\link.log]

WHY THIS EXISTS
---------------
`AGENTS.md` requires a known-signature frontier to diagnose, flush and stop. A frontier that is
DECLARED and never DEFINED does none of that: under `/FORCE` the linker resolves it to address zero
and the call becomes an unnamed access violation. That is strictly worse than a named abort, and it
is worse still when the declaration carries a comment promising a stop - the comment is exactly what
stops anyone from checking.

On 2026-09-09 four of these were found. Three (`SimpleObject::initAiObjectAndPortal`,
`aiObjectCreateWithInstance`, `enableAiObjectAndPortal`) were found by CRASHING INTO ONE: the AI
started initialising, `isBotsGame()` became true for the first time, and a coop load jumped to RVA 0.
The fourth (`portalManagerUpdate`, called every tick of a bots game) was found by this script - and
only after it was extended to cover FREE FUNCTIONS, which is how it had survived four earlier passes.

WHAT IT REPORTS
---------------
  1. members and free functions that are declared with a frontier marker and defined nowhere;
  2. the subset of those sitting behind an AI/bots gate, i.e. reachable now that `BF2AI::init`
     succeeds. That subset is the one to fix first, and it is the shape that crashed.

Pass a link.log to also cross-check against the image's actual unresolved externals; without one the
source scan alone is used, which is the more complete of the two (an unresolved-externals list only
contains symbols something REFERENCES).

⚠ The definition test is textual, not a compiler. Treat a hit as "look at this", not as proof.
"""
import io
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "BF2")
ROOT = os.path.normpath(ROOT)

MARKERS = ("[FRONTIER]", "no body", "no bodies", "unwritten", "BY NAME")
AI_GATE = re.compile(r"isBotsGame|g_aiInstance|aiInstance\(\)|isAIGame")

DECL = re.compile(
    r"^(?!\s*(?:return|if|for|while|switch|else|case)\b)"
    r"[ \t]*(?:extern[ \t]+)?(?:static[ \t]+)?(?:virtual[ \t]+)?"
    r"(?:const[ \t]+)?[A-Za-z_][A-Za-z0-9_:<>, \t\*&]*?[ \t\*&]"
    r"([A-Za-z_][A-Za-z0-9_]*)[ \t]*\([^;{)]*\)[ \t]*(?:const[ \t]*)?;",
    re.M)


def load():
    src, hdr = {}, {}
    for base, _dirs, files in os.walk(ROOT):
        if os.sep + "External" + os.sep in base + os.sep:
            continue
        for fn in files:
            if not fn.endswith((".cpp", ".h", ".hpp")):
                continue
            p = os.path.join(base, fn)
            try:
                t = io.open(p, "rb").read().decode("utf-8", "replace")
            except Exception:
                continue
            (src if fn.endswith(".cpp") else hdr)[p] = t
    return src, hdr


def main():
    src, hdr = load()
    cache = {}

    def defined(name):
        if name not in cache:
            free = re.compile(r"(?<![:\w])" + re.escape(name) + r"[ \t]*\([^;]*?\)[^;{]*\{", re.S)
            memb = re.compile(r"::[ \t]*" + re.escape(name) + r"[ \t]*\(")
            cache[name] = any(free.search(t) or memb.search(t) for t in src.values())
        return cache[name]

    found = []
    for p, t in list(src.items()) + list(hdr.items()):
        lines = t.split("\n")
        for m in DECL.finditer(t):
            name = m.group(1)
            ln = t[:m.start()].count("\n") + 1
            own = lines[ln - 1]
            near = "\n".join(lines[max(0, ln - 6):ln])
            if not any(k in own or k in near for k in MARKERS):
                continue
            if defined(name):
                continue
            found.append((name, os.path.relpath(p, ROOT), ln, own.strip()[:100]))
    found = sorted(set(found))

    names = {f[0] for f in found}
    gated = set()
    for p, t in src.items():
        lines = t.split("\n")
        for i, line in enumerate(lines):
            if not AI_GATE.search(line):
                continue
            for j in range(i, min(i + 14, len(lines))):
                code = lines[j].split("//")[0]
                for n in names:
                    if re.search(r"(?<![:\w])" + re.escape(n) + r"[ \t]*\(", code):
                        gated.add((n, os.path.relpath(p, ROOT), j + 1, lines[j].strip()[:88]))

    print("=" * 78)
    print("FRONTIER DECLARATIONS WITH NO DEFINITION  (RVA 0 under /FORCE)")
    print("=" * 78)
    last = None
    for n, p, ln, text in found:
        if p != last:
            print()
            print(p)
            last = p
        print("  %5d  %-36s %s" % (ln, n, text))
    print()
    print("total: %d in %d files" % (len(found), len({f[1] for f in found})))

    print()
    print("=" * 78)
    print("...BEHIND AN AI / BOTS GATE  -> reachable now, fix these first")
    print("=" * 78)
    for n, p, ln, text in sorted(gated):
        print("  %-34s %s:%d" % (n, p, ln))
        print("        %s" % text)
    print()
    print("total: %d" % len(gated))
    return 0


if __name__ == "__main__":
    sys.exit(main())
