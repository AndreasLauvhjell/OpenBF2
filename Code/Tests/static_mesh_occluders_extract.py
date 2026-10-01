# -*- coding: utf-8 -*-
# [TEMP] SCAFFOLDING - extracts the stock .occ files so static_mesh_occluders_runtime.exe can read
# them as loose files. Idempotent; names are flattened and de-duplicated by archive.
import os
import sys
import zipfile

if len(sys.argv) < 3:
    sys.stderr.write("usage: static_mesh_occluders_extract.py <game-dir> <out-dir>\n")
    raise SystemExit(2)

gameDir = sys.argv[1]
outDir = sys.argv[2]

if not os.path.isdir(gameDir):
    sys.stderr.write("game dir not found: %s\n" % gameDir)
    raise SystemExit(2)
if not os.path.isdir(outDir):
    os.makedirs(outDir)

written = 0
for dirpath, dirnames, filenames in os.walk(gameDir):
    for filename in filenames:
        if not filename.lower().endswith(".zip"):
            continue
        archivePath = os.path.join(dirpath, filename)
        try:
            archive = zipfile.ZipFile(archivePath)
        except Exception:
            continue
        tag = os.path.splitext(filename)[0]
        for entry in archive.namelist():
            if not entry.lower().endswith(".occ"):
                continue
            base = "%s__%s" % (tag, os.path.basename(entry))
            target = os.path.join(outDir, base)
            if os.path.exists(target):
                continue
            with open(target, "wb") as handle:
                handle.write(archive.read(entry))
            written += 1
        archive.close()

print("extracted %d new .occ file(s) into %s" % (written, outDir))
