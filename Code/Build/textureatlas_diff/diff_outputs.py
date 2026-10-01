# [TEMP] [CHOSEN, not recovered] - SCAFFOLDING. Compares one scenario's output of the retail
# TextureAtlasBuilder.dll with the rebuilt one's (textureatlas_diff.cmd).
#
#   python diff_outputs.py <retail dir> <rebuilt dir> [<retail log> <rebuilt log>]
#
#   * the host logs (every callback message, every value the interface returned) line by line;
#   * .tai / .tac byte for byte (the first differing line is printed);
#   * .dds: the 128-byte headers, then each mip level. A level that differs is re-compared INSIDE the
#     packed rectangles only (read back from the page's .tai lines; for DXT pages, the 4 x 4 blocks
#     wholly inside a rectangle): a texel no texture covers is never written by the DLL, so a SYSTEMMEM
#     gutter may hold allocator garbage (census section 4.7, [UNRESOLVED]) - a difference confined to
#     the gutters is reported as such, not as a failure.
#
# Exit code 0 = identical (gutters aside), 1 = a difference that matters, 2 = usage.

import os
import struct
import sys


def read(path):
    with open(path, "rb") as f:
        return f.read()


def first_line_diff(a, b):
    la, lb = a.split(b"\n"), b.split(b"\n")
    for i in range(max(len(la), len(lb))):
        x = la[i] if i < len(la) else b"<missing>"
        y = lb[i] if i < len(lb) else b"<missing>"
        if x != y:
            return "line %d:\n      retail  %r\n      rebuilt %r" % (i + 1, x, y)
    return "identical lines (line endings differ?)"


def dds_info(data):
    if len(data) < 128 or data[:4] != b"DDS ":
        return None
    h, w = struct.unpack_from("<II", data, 12)
    mips = struct.unpack_from("<I", data, 28)[0] or 1
    pf_flags, fourcc, bits = struct.unpack_from("<I4sI", data, 80)
    if pf_flags & 0x4:
        fmt = fourcc.decode("latin1")
    else:
        fmt = "RGB%d" % bits
    return w, h, mips, fmt


def level_layout(w, h, mips, fmt):
    out, offset = [], 128
    for level in range(mips):
        lw, lh = max(1, w >> level), max(1, h >> level)
        if fmt.startswith("DXT"):
            block = 8 if fmt == "DXT1" else 16
            size = max(1, (lw + 3) // 4) * max(1, (lh + 3) // 4) * block
        else:
            size = lw * lh * (int(fmt[3:]) // 8)
        out.append((level, lw, lh, offset, size))
        offset += size
    return out


def tai_rects(tai_text, page_file):
    """Pixel rectangles (level 0) of every .tai line naming page_file."""
    rects = []
    for raw in tai_text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "\t\t" not in line:
            continue
        _, rest = line.split("\t\t", 1)
        fields = [f.strip() for f in rest.split(",")]
        if len(fields) < 6 or os.path.basename(fields[0].replace("\\", "/")) != page_file:
            continue
        u, v, w, h = (float(x) for x in fields[2:6])
        rects.append((u, v, w, h))
    return rects


def masked_compare(a, b, layout_entry, fmt, rects, w0, h0):
    level, lw, lh, offset, size = layout_entry
    scale = 1 << level
    differ = 0
    if fmt.startswith("DXT"):
        block = 8 if fmt == "DXT1" else 16
        bw = max(1, (lw + 3) // 4)
        for (u, v, rw, rh) in rects:
            x0 = int(round(u * w0)) // scale
            y0 = int(round(v * h0)) // scale
            x1 = int(round((u + rw) * w0)) // scale
            y1 = int(round((v + rh) * h0)) // scale
            for by in range((y0 + 3) // 4, y1 // 4):
                for bx in range((x0 + 3) // 4, x1 // 4):
                    o = offset + (by * bw + bx) * block
                    if a[o:o + block] != b[o:o + block]:
                        differ += 1
    else:
        bpp = int(fmt[3:]) // 8
        for (u, v, rw, rh) in rects:
            x0 = int(round(u * w0)) // scale
            y0 = int(round(v * h0)) // scale
            x1 = int(round((u + rw) * w0)) // scale
            y1 = int(round((v + rh) * h0)) // scale
            for y in range(y0, y1):
                o = offset + (y * lw + x0) * bpp
                if a[o:o + (x1 - x0) * bpp] != b[o:o + (x1 - x0) * bpp]:
                    differ += 1
    return differ


def compare_dds(name, a, b, tai_texts):
    ia, ib = dds_info(a), dds_info(b)
    if not ia or not ib:
        return "not a DDS on one side", True
    if a[:128] != b[:128]:
        return "headers differ: retail %s, rebuilt %s" % (ia, ib), True
    if a == b:
        return None, False
    w, h, mips, fmt = ia
    rects = []
    for text in tai_texts:
        rects += tai_rects(text, name)
    notes, matters = [], False
    for entry in level_layout(w, h, mips, fmt):
        level, lw, lh, offset, size = entry
        if a[offset:offset + size] == b[offset:offset + size]:
            continue
        if not rects:
            notes.append("level %d differs (no .tai rectangles to mask with)" % level)
            matters = True
            continue
        inside = masked_compare(a, b, entry, fmt, rects, w, h)
        if inside:
            notes.append("level %d differs INSIDE packed rectangles (%d rows/blocks)" % (level, inside))
            matters = True
        else:
            notes.append("level %d differs only in the gutters" % level)
    return "; ".join(notes), matters


def main(argv):
    if len(argv) not in (3, 5):
        print(__doc__ if __doc__ else "usage")
        return 2
    left, right = argv[1], argv[2]
    failures, notes = 0, 0

    if len(argv) == 5:
        la, lb = read(argv[3]), read(argv[4])
        # The first line names the device/string mode; strip it from the comparison.
        la = b"\n".join(l for l in la.split(b"\n") if not l.startswith(b"[host] device:"))
        lb = b"\n".join(l for l in lb.split(b"\n") if not l.startswith(b"[host] device:"))
        if la != lb:
            print("  LOG  differs - " + first_line_diff(la, lb))
            failures += 1

    names = set()
    for base in (left, right):
        if os.path.isdir(base):
            for dirpath, _, files in os.walk(base):
                for f in files:
                    names.add(os.path.relpath(os.path.join(dirpath, f), base))
    tai_texts = []
    for f in sorted(names):
        if f.lower().endswith(".tai") and os.path.exists(os.path.join(left, f)):
            tai_texts.append(read(os.path.join(left, f)).decode("latin1"))

    for f in sorted(names):
        pa, pb = os.path.join(left, f), os.path.join(right, f)
        if not os.path.exists(pa) or not os.path.exists(pb):
            print("  FILE %s only in %s" % (f, "retail" if os.path.exists(pa) else "rebuilt"))
            failures += 1
            continue
        a, b = read(pa), read(pb)
        if f.lower().endswith(".dds"):
            text, matters = compare_dds(os.path.basename(f), a, b, tai_texts)
            if text:
                print("  DDS  %s: %s" % (f, text))
                if matters:
                    failures += 1
                else:
                    notes += 1
        elif a != b:
            print("  FILE %s differs - %s" % (f, first_line_diff(a, b)))
            failures += 1

    if failures:
        print("DIFFERENT: %d difference(s)%s" % (failures, (", %d gutter-only note(s)" % notes) if notes else ""))
        return 1
    print("IDENTICAL%s (%d file(s))" % ((" except %d gutter-only note(s)" % notes) if notes else "", len(names)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
