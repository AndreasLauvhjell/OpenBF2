# [TEMP] [CHOSEN, not recovered] - SCAFFOLDING. Writes the synthetic inputs of the TextureAtlasBuilder
# parity harness (textureatlas_diff.cmd): DDS files in the three formats the stock atlases use
# (DXT1, DXT5, A8R8G8B8), with and without mip chains, power-of-two and not, one 4096 x 4096 (larger
# than any page), and the .tac files of scenarios S9 and S11. Deterministic: every texel is drawn from
# a generator seeded by the file's own name, so the retail and the rebuilt runs read identical bytes.
#
#   python make_inputs.py <inputs dir>
#
# No game data is read or copied.

import os
import random
import struct
import sys
import zlib


def mip_count(w, h):
    n = 1
    while max(w, h) > 1:
        w, h = max(1, w // 2), max(1, h // 2)
        n += 1
    return n


def level_size(fmt, w, h):
    if fmt in ("DXT1", "DXT5"):
        block = 8 if fmt == "DXT1" else 16
        return max(1, (w + 3) // 4) * max(1, (h + 3) // 4) * block
    return w * h * 4


def write_dds(path, w, h, fmt, mips):
    """fmt: 'DXT1', 'DXT5' or 'ARGB' (A8R8G8B8); mips: 1, or 0 for the full chain."""
    if mips == 0:
        mips = mip_count(w, h)
    compressed = fmt in ("DXT1", "DXT5")
    flags = 0x1 | 0x2 | 0x4 | 0x1000                        # CAPS HEIGHT WIDTH PIXELFORMAT
    flags |= 0x80000 if compressed else 0x8                  # LINEARSIZE / PITCH
    if mips > 1:
        flags |= 0x20000                                     # MIPMAPCOUNT
    pitch = level_size(fmt, w, h) if compressed else w * 4
    if compressed:
        pf = struct.pack("<II4sIIIII", 32, 0x4, fmt.encode("ascii"), 0, 0, 0, 0, 0)
    else:
        pf = struct.pack("<IIIIIIII", 32, 0x40 | 0x1, 0, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)
    caps = 0x1000 | ((0x8 | 0x400000) if mips > 1 else 0)
    header = struct.pack("<4sIIIIIII", b"DDS ", 124, flags, h, w, pitch, 0, mips)
    header += b"\0" * 44 + pf + struct.pack("<IIIII", caps, 0, 0, 0, 0)
    assert len(header) == 128
    rng = random.Random(zlib.crc32(path.replace("\\", "/").lower().encode("ascii")))
    body = bytearray()
    lw, lh = w, h
    for _ in range(mips):
        body += rng.randbytes(level_size(fmt, lw, lh))
        lw, lh = max(1, lw // 2), max(1, lh // 2)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(header + bytes(body))


def write_text(path, lines):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(("\r\n".join(lines) + "\r\n").encode("ascii"))


def main(root):
    j = lambda *parts: os.path.join(root, *parts)

    # S1 - overgrowth: a DXT5 group fed mixed sources.
    write_dds(j("og", "Bush_A.dds"), 128, 128, "DXT5", 0)
    write_dds(j("og", "bush_b.dds"), 128, 128, "DXT5", 0)
    write_dds(j("og", "Tree_Pine.dds"), 256, 128, "DXT1", 0)
    write_dds(j("og", "tree_leaves.dds"), 64, 64, "DXT5", 1)
    write_dds(j("og", "Stump.dds"), 64, 64, "DXT1", 0)
    write_dds(j("og", "grass_rgba.dds"), 64, 32, "ARGB", 0)

    # S2 - undergrowth: equal sizes, CWD-relative names, DXT5 group.
    for i in range(5):
        write_dds(j("ug", "grass%02d.dds" % i), 64, 64, "DXT1", 0)

    # S3 - decals under an input directory: format "" (split runs).
    sizes = [(32, 32), (64, 64), (32, 32), (16, 16), (64, 32), (32, 32)]
    for i, (w, h) in enumerate(sizes):
        write_dds(j("decal_dir", "decal", "hole%02d.dds" % i), w, h, "ARGB", 0)
    write_dds(j("decal_dir", "decal", "scorch_big.dds"), 128, 128, "DXT5", 0)

    # S4 - particles: a format mix and two non-power-of-two sources.
    kinds = [("DXT5", 64, 64), ("DXT1", 32, 32), ("ARGB", 48, 48), ("DXT5", 64, 64), ("DXT1", 128, 64),
             ("ARGB", 32, 32), ("DXT5", 32, 32), ("ARGB", 96, 32), ("DXT1", 64, 64)]
    for i, (fmt, w, h) in enumerate(kinds):
        write_dds(j("particle", "fx%02d.dds" % i), w, h, fmt, 0)

    # S5 - object lightmaps: sixteen 256 x 256 and four 128 x 128, all DXT1 (the tie case).
    for i in range(20):
        size = 256 if i < 16 else 128
        write_dds(j("lightmaps", "lm%02d.dds" % i), size, size, "DXT1", 0)

    # S6 - 48 equal-area sources: the VC7.1 sort decides every placement.
    for i in range(48):
        write_dds(j("ties", "t%02d.dds" % i), 64, 64, "ARGB", 1)

    # S7 - more than one page.
    for i in range(6):
        write_dds(j("pages", "big%02d.dds" % i), 1024, 1024, "DXT1", 1)

    # S8 - mixed formats, format "": three runs.
    for i in range(9):
        fmt = ("ARGB", "DXT1", "DXT5")[i % 3]
        write_dds(j("mixed", "m%02d.dds" % i), 64, 64, fmt, 0)

    # S10 - sort mode 1 (by size): shapes that order differently by side and by area.
    shapes = [(64, 32), (32, 64), (64, 64), (128, 16), (16, 128), (32, 32), (64, 32), (32, 64),
              (48, 48), (128, 32), (32, 128), (16, 16)]
    for i, (w, h) in enumerate(shapes):
        write_dds(j("sizes", "s%02d.dds" % i), w, h, "ARGB", 1)

    # S9 - a MemeAtlas-shaped .tac: width/height/format/miplevels/border, bordered textures.
    meme = []
    for i in range(8):
        size = 32 if i % 2 else 64
        write_dds(j("meme", "icon%02d.dds" % i), size, size, "ARGB", 1)
        meme.append("meme/icon%02d.dds%s" % (i, " border" if i < 5 else ""))
    write_text(j("tac", "meme.tac"),
               ["# Texture Atlas Configuration file",
                "#",
                "group MemeAtlas width 2048 height 2048 format dxt5 miplevels 1 border 2"] + meme + [""])

    # S11 - the error paths.
    write_dds(j("errors", "small.dds"), 16, 16, "ARGB", 1)
    write_dds(j("errors", "huge.dds"), 4096, 4096, "DXT1", 1)
    write_text(j("tac", "flags.tac"),
               ["# a texture before any group, unknown flags, a non-numeric border",
                "errors/small.dds border",
                "group FlagGroup width 512 frobnicate 7",
                "errors/small.dds border extra",
                "group Second height 128 border x",
                "errors/small.dds",
                ""])
    write_text(j("tac", "noname.tac"),
               ["group #nameless width 64",
                "errors/small.dds",
                ""])
    print("inputs written to", root)


if __name__ == "__main__":
    main(sys.argv[1])
