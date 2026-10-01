#!/usr/bin/env python3
"""SCAFFOLDING (Code/Tests) - dump the display-list tags (PlaceObject/PlaceObject2/RemoveObject/RemoveObject2)
per frame of a sprite in an uncompressed SWF, plus the simulated depth->character state after each
keyframe, so a goto's expected removals/placements can be read off the file.

  usage: swfdump.py <swf> --frames N        every DefineSprite with N frames
         swfdump.py <swf> --id ID           one DefineSprite by character id
         swfdump.py <swf> --root            the main timeline
"""
import struct, sys


class BitReader:
    def __init__(self, b, p):
        self.b = b; self.p = p; self.bit = 0

    def u(self, n):
        v = 0
        for _ in range(n):
            byte = self.b[self.p]
            v = (v << 1) | ((byte >> (7 - self.bit)) & 1)
            self.bit += 1
            if self.bit == 8:
                self.bit = 0; self.p += 1
        return v

    def s(self, n):
        v = self.u(n)
        if n and v & (1 << (n - 1)):
            v -= 1 << n
        return v

    def align(self):
        if self.bit:
            self.bit = 0; self.p += 1


def read_matrix(b, p):
    r = BitReader(b, p)
    sx = sy = 1.0; rot = None
    if r.u(1):
        n = r.u(5); sx = r.s(n) / 65536.0; sy = r.s(n) / 65536.0
    if r.u(1):
        n = r.u(5); rot = (r.s(n) / 65536.0, r.s(n) / 65536.0)
    n = r.u(5); tx = r.s(n); ty = r.s(n)
    r.align()
    return (sx, sy, rot, tx, ty), r.p


def skip_cxform(b, p, alpha):
    r = BitReader(b, p)
    hasadd = r.u(1); hasmul = r.u(1); n = r.u(5)
    cnt = 4 if alpha else 3
    if hasmul:
        for _ in range(cnt): r.s(n)
    if hasadd:
        for _ in range(cnt): r.s(n)
    r.align()
    return r.p


def tags(b, off, end):
    while off < end:
        h = struct.unpack_from('<H', b, off)[0]
        code, ln = h >> 6, h & 0x3F
        off += 2
        if ln == 0x3F:
            ln = struct.unpack_from('<I', b, off)[0]; off += 4
        yield code, off, off + ln
        off += ln


def cstr(b, p):
    e = b.index(b'\0', p)
    return b[p:e].decode('latin1'), e + 1


TAGNAMES = {2: 'Shape', 22: 'Shape2', 32: 'Shape3', 7: 'Button', 34: 'Button2', 11: 'Text', 33: 'Text2',
            37: 'EditText', 39: 'Sprite', 20: 'BitsLossless', 36: 'BitsLossless2', 21: 'JPEG2', 35: 'JPEG3',
            46: 'MorphShape', 10: 'Font', 48: 'Font2', 13: 'FontInfo'}


def parse_place(b, p, end, code, ver):
    if code == 4:
        cid, depth = struct.unpack_from('<HH', b, p); p += 4
        m, p = read_matrix(b, p)
        return dict(kind='PLACE', depth=depth, id=cid, matrix=m, name=None)
    flags = b[p]; p += 1
    has_actions = flags & 0x80; has_clip = flags & 0x40; has_name = flags & 0x20; has_ratio = flags & 0x10
    has_cx = flags & 0x08; has_mat = flags & 0x04; has_char = flags & 0x02; move = flags & 0x01
    depth = struct.unpack_from('<H', b, p)[0]; p += 2
    cid = None; m = None; name = None; ratio = None; clip = None; ev = 0
    if has_char:
        cid = struct.unpack_from('<H', b, p)[0]; p += 2
    if has_mat:
        m, p = read_matrix(b, p)
    if has_cx:
        p = skip_cxform(b, p, True)
    if has_ratio:
        ratio = struct.unpack_from('<H', b, p)[0]; p += 2
    if has_name:
        name, p = cstr(b, p)
    if has_clip:
        clip = struct.unpack_from('<H', b, p)[0]; p += 2
    if has_actions:
        ev = 1
    if has_char and move:
        kind = 'REPLACE'
    elif move:
        kind = 'MOVE'
    else:
        kind = 'PLACE'
    return dict(kind=kind, depth=depth, id=cid, matrix=m, name=name, ratio=ratio, clip=clip, ev=ev)


def dump_timeline(b, start, end, ver, defs, exports, label):
    frame = 0
    ops = []          # (frame, op)
    labels = {}
    for code, p, e in tags(b, start, end):
        if code == 1:
            frame += 1
        elif code in (4, 26):
            ops.append((frame, parse_place(b, p, e, code, ver)))
        elif code == 5:
            cid, depth = struct.unpack_from('<HH', b, p)
            ops.append((frame, dict(kind='REMOVE', depth=depth, id=cid)))
        elif code == 28:
            depth = struct.unpack_from('<H', b, p)[0]
            ops.append((frame, dict(kind='REMOVE', depth=depth, id=None)))
        elif code == 43:
            labels[frame] = cstr(b, p)[0]
        elif code == 12:
            ops.append((frame, dict(kind='ACTION', depth=None, id=None, size=e - p)))
        elif code == 59:
            ops.append((frame, dict(kind='INITACTION', depth=None, id=None, size=e - p)))
    print('=== %s: %d frames, labels %s' % (label, frame, labels))
    state = {}
    byframe = {}
    for f, op in ops:
        byframe.setdefault(f, []).append(op)
    for f in sorted(byframe):
        print('-- frame %d %s' % (f, ('label=' + labels[f]) if f in labels else ''))
        for op in byframe[f]:
            k = op['kind']
            if k in ('ACTION', 'INITACTION'):
                print('   %-8s %d bytes' % (k, op['size']))
                continue
            d = op['depth']
            desc = ''
            if op.get('id') is not None:
                cid = op['id']
                desc = 'id=%d(%s%s)' % (cid, TAGNAMES.get(defs.get(cid, -1), str(defs.get(cid))),
                                        (' ' + exports[cid]) if cid in exports else '')
            m = op.get('matrix')
            mdesc = ''
            if m:
                sx, sy, rot, tx, ty = m
                mdesc = ' at (%.1f,%.1f)px' % (tx / 20.0, ty / 20.0)
                if sx != 1.0 or sy != 1.0:
                    mdesc += ' scale(%.3f,%.3f)' % (sx, sy)
            nm = (" name='%s'" % op['name']) if op.get('name') else ''
            ex = ' +clipevents' if op.get('ev') else ''
            ex += (' clip=%d' % op['clip']) if op.get('clip') else ''
            prev = state.get(d)
            pdesc = (' [was id=%s %s]' % (prev[0], prev[1] or '')) if prev else ''
            print('   %-8s depth=%-3d %s%s%s%s%s' % (k, d, desc, nm, mdesc, ex, pdesc))
            if k == 'PLACE' or k == 'REPLACE':
                state[d] = (op['id'], op['name'])
            elif k == 'MOVE':
                if d in state:
                    state[d] = (state[d][0], state[d][1] or op.get('name'))
            elif k == 'REMOVE':
                state.pop(d, None)
        print('   state after frame %d: %d chars: %s' % (
            f, len(state), ' '.join('%d:%d%s' % (d, v[0], ('/' + v[1]) if v[1] else '') for d, v in sorted(state.items()))))


def census_timeline(b, start, end, ver, defs, exports, label, out):
    """Count PLACE tags landing on an occupied depth: same character id or not, and whether a
    REMOVE of that depth preceded it in the same frame (Flash: always a new instance)."""
    frame = 0
    state = {}
    removed_this_frame = {}
    for code, p, e in tags(b, start, end):
        if code == 1:
            frame += 1
            removed_this_frame = {}
        elif code in (4, 26):
            op = parse_place(b, p, e, code, ver)
            d = op['depth']
            if op['kind'] == 'PLACE':
                if d in state:
                    out.append((label, frame, d, op['id'], state[d][0] == op['id'], False, op['name']))
                elif d in removed_this_frame:
                    out.append((label, frame, d, op['id'], removed_this_frame[d][0] == op['id'], True, op['name']))
                state[d] = (op['id'], op['name'])
            elif op['kind'] == 'REPLACE':
                state[d] = (op['id'], op['name'])
        elif code == 5:
            cid, depth = struct.unpack_from('<HH', b, p)
            if depth in state: removed_this_frame[depth] = state.pop(depth)
        elif code == 28:
            depth = struct.unpack_from('<H', b, p)[0]
            if depth in state: removed_this_frame[depth] = state.pop(depth)


def main():
    path = sys.argv[1]
    b = open(path, 'rb').read()
    assert b[:3] == b'FWS', 'compressed SWF not supported'
    ver = b[3]
    p = 8
    nbits = b[p] >> 3
    p += (5 + 4 * nbits + 7) // 8
    rate, count = struct.unpack_from('<HH', b, p); p += 4
    print('SWF v%d, %d root frames, %.2f fps' % (ver, count, rate / 256.0))
    defs = {}
    exports = {}
    sprites = []
    for code, s, e in tags(b, p, len(b)):
        if code in TAGNAMES or code in (39,):
            cid = struct.unpack_from('<H', b, s)[0]
            defs[cid] = code
        if code == 39:
            cid, fc = struct.unpack_from('<HH', b, s)
            sprites.append((cid, fc, s + 4, e))
        if code == 56:
            n = struct.unpack_from('<H', b, s)[0]; q = s + 2
            for _ in range(n):
                cid = struct.unpack_from('<H', b, q)[0]; q += 2
                name, q = cstr(b, q)
                exports[cid] = name
    mode = sys.argv[2]
    if mode == '--root':
        dump_timeline(b, p, len(b), ver, defs, exports, 'ROOT')
    elif mode == '--frames':
        n = int(sys.argv[3])
        for cid, fc, s, e in sprites:
            if fc == n:
                dump_timeline(b, s, e, ver, defs, exports, 'sprite %d%s' % (cid, (' ' + exports[cid]) if cid in exports else ''))
    elif mode == '--id':
        want = int(sys.argv[3])
        for cid, fc, s, e in sprites:
            if cid == want:
                dump_timeline(b, s, e, ver, defs, exports, 'sprite %d%s' % (cid, (' ' + exports[cid]) if cid in exports else ''))
    elif mode == '--list':
        for cid, fc, s, e in sprites:
            print('sprite %d frames=%d bytes=%d %s' % (cid, fc, e - s, exports.get(cid, '')))
    elif mode == '--census':
        out = []
        census_timeline(b, p, len(b), ver, defs, exports, 'ROOT', out)
        for cid, fc, s, e in sprites:
            census_timeline(b, s, e, ver, defs, exports, 'sprite %d' % cid, out)
        tally = {}
        for label, frame, d, cid, same, pre, name in out:
            key = ('REMOVE+PLACE' if pre else 'PLACE over live', 'same id' if same else 'other id')
            tally[key] = tally.get(key, 0) + 1
            print('%-14s frame %-3d depth %-3d id %-4s %s %s%s' % (
                label, frame, d, cid, 'REMOVE+PLACE' if pre else 'PLACE-over-live',
                'SAME-ID' if same else 'other-id', (" name='%s'" % name) if name else ''))
        print('--- tally:')
        for k, v in sorted(tally.items()):
            print('   %-16s %-8s %d' % (k[0], k[1], v))


if __name__ == '__main__':
    main()
