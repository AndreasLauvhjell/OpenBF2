#!/usr/bin/env python3
"""
Validate OldConsole::include / ::run path resolution against the shipped BF2 content.

The rule under test is the one recovered from ELF 0x844E9E0 (include) and 0x844D980 (run)
on 2026-08-14:

    fileName = name
    if '.' not in name:                       fileName += ".inc"   (include) / ".con" (run)
    if m_useRelativePaths:                    # BF2Engine sets this true at init
        if fileName[0] in "/\\":              fileName = fileName[1:]      # strip, do NOT prefix
        else:                                 fileName = m_workingPath + fileName
    fixPathDelimiters(fileName, '/')

m_workingPath is set by run/include to the directory of the script currently executing, so for a
statement inside <root>/a/b/c.con the working path is "a/b/".

We compare it against the behaviour the landed run() had BEFORE the fix, which omitted the whole
m_useRelativePaths block, i.e. used the bare name.
"""
import os
import re
import sys
from collections import Counter

# The shipped-content corpus. Override with argv[1] or BF2_CONTENT_ROOT; this default is the
# install this reconstruction has been validated against.
ROOT = os.environ.get("BF2_CONTENT_ROOT", r"D:\B2BF Launcher\Battlefield2\mods\bf2")
STMT = re.compile(r'^\s*(include|run)\s+([^\s;]+)', re.IGNORECASE)
EXTS = (".con", ".tweak", ".inc")


# From <root>/ServerArchives.con - the script BF2Engine runs through runFullAccess before any
# content loads. Each archive is mounted AT a virtual directory, so an entry stored in the zip as
# "staticobjects/x.con" is visible to File::open as "Objects/staticobjects/x.con".
MOUNTS = [
    ("Objects_server.zip", "Objects"),
    ("Menu_server.zip", "Menu"),
    ("Common_server.zip", "Common"),
    ("Booster_server.zip", "Objects"),
]


def build_index(root):
    """relative posix lowercase path -> True, for loose files AND mounted archive entries."""
    import zipfile

    index = set()
    for dirpath, _dirnames, filenames in os.walk(root):
        rel = os.path.relpath(dirpath, root).replace("\\", "/")
        prefix = "" if rel == "." else rel + "/"
        for fn in filenames:
            index.add((prefix + fn).lower())

    loose = len(index)
    for zip_name, mount in MOUNTS:
        zip_path = os.path.join(root, zip_name)
        if not os.path.isfile(zip_path):
            continue
        try:
            with zipfile.ZipFile(zip_path) as zf:
                for entry in zf.namelist():
                    if entry.endswith("/"):
                        continue
                    index.add(("%s/%s" % (mount, entry)).replace("\\", "/").lower())
        except (OSError, zipfile.BadZipFile):
            continue
    print("  loose files: %d, plus mounted archive entries: %d" % (loose, len(index) - loose))
    return index


def resolve(name, working_path, default_ext):
    """The recovered rule."""
    file_name = name
    if "." not in file_name:
        file_name = file_name + default_ext
    # m_useRelativePaths is true on the dedicated server (BF2Engine::initEngine)
    if file_name[:1] in ("/", "\\"):
        file_name = file_name[1:]
    else:
        file_name = working_path + file_name
    return file_name.replace("\\", "/")


def legacy(name, default_ext):
    """What run() did before the fix: no working-path resolution at all."""
    file_name = name
    if "." not in file_name:
        file_name = file_name + default_ext
    return file_name.replace("\\", "/")


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else ROOT
    if not os.path.isdir(root):
        print("corpus not found: %s" % root)
        print("pass the mods/bf2 path as argv[1] or set BF2_CONTENT_ROOT")
        return 1

    index = build_index(root)
    print("indexed %d files under %s" % (len(index), root))

    stats = Counter()
    miss_examples = []
    tweak_misses = 0

    for dirpath, _dirnames, filenames in os.walk(root):
        rel_dir = os.path.relpath(dirpath, root).replace("\\", "/")
        working_path = "" if rel_dir == "." else rel_dir + "/"
        for fn in filenames:
            if not fn.lower().endswith(EXTS):
                continue
            path = os.path.join(dirpath, fn)
            try:
                with open(path, "r", encoding="latin-1") as fh:
                    lines = fh.readlines()
            except OSError:
                continue

            for line in lines:
                m = STMT.match(line)
                if not m:
                    continue
                kind = m.group(1).lower()
                raw = m.group(2).strip().strip('"')
                if not raw:
                    continue
                default_ext = ".inc" if kind == "include" else ".con"

                stats[kind + "_total"] += 1
                r = resolve(raw, working_path, default_ext)
                l = legacy(raw, default_ext)

                if r.lower() in index:
                    stats[kind + "_resolved_hit"] += 1
                else:
                    stats[kind + "_resolved_miss"] += 1
                    if r.lower().endswith(".tweak"):
                        tweak_misses += 1
                    elif len(miss_examples) < 15:
                        miss_examples.append(
                            "%s: %s %s -> %s" % (working_path or "<root>", kind, raw, r))

                if l.lower() in index:
                    stats[kind + "_legacy_hit"] += 1
                else:
                    stats[kind + "_legacy_miss"] += 1

    print()
    for kind in ("include", "run"):
        total = stats[kind + "_total"]
        if not total:
            continue
        rh = stats[kind + "_resolved_hit"]
        lh = stats[kind + "_legacy_hit"]
        print("%-8s statements: %5d" % (kind, total))
        print("    recovered rule  : %5d hit  (%5.1f%%)  %5d miss"
              % (rh, 100.0 * rh / total, total - rh))
        print("    pre-fix (no res): %5d hit  (%5.1f%%)  %5d miss"
              % (lh, 100.0 * lh / total, total - lh))
    print()
    print("misses whose target is a .tweak (expected: silent success per the recovered "
          "open-failure branch): %d" % tweak_misses)
    if miss_examples:
        print()
        print("non-tweak misses (sample):")
        for e in miss_examples:
            print("   " + e)
    return 0


if __name__ == "__main__":
    sys.exit(main())
