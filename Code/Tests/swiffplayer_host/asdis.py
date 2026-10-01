#!/usr/bin/env python3
"""SCAFFOLDING (Code/Tests) - a minimal AS2 (SWF 7) disassembler for the menu movies.

Walks an uncompressed SWF, collects every action block (DoAction 12, DoInitAction 59, the PlaceObject2
clip actions 26, DefineButton2 40 actions, recursing into DefineSprite 39), decodes the action records and
prints the blocks (or only the DefineFunction bodies) that mention the strings asked for.

  usage: asdis.py <swf> <needle> [<needle> ...]        blocks containing any needle
         asdis.py <swf> --all                          every block
         asdis.py <swf> --strings                      constant-pool / push strings, counted
"""
import struct, sys

OPS = {
    0x04: 'NextFrame', 0x05: 'PrevFrame', 0x06: 'Play', 0x07: 'Stop', 0x08: 'ToggleQuality', 0x09: 'StopSounds',
    0x0A: 'Add', 0x0B: 'Subtract', 0x0C: 'Multiply', 0x0D: 'Divide', 0x0E: 'Equals', 0x0F: 'Less', 0x10: 'And',
    0x11: 'Or', 0x12: 'Not', 0x13: 'StringEquals', 0x14: 'StringLength', 0x15: 'StringExtract', 0x17: 'Pop',
    0x18: 'ToInteger', 0x1C: 'GetVariable', 0x1D: 'SetVariable', 0x20: 'SetTarget2', 0x21: 'StringAdd',
    0x22: 'GetProperty', 0x23: 'SetProperty', 0x24: 'CloneSprite', 0x25: 'RemoveSprite', 0x26: 'Trace',
    0x27: 'StartDrag', 0x28: 'EndDrag', 0x29: 'StringLess', 0x2A: 'Throw', 0x2B: 'CastOp', 0x2C: 'ImplementsOp',
    0x30: 'RandomNumber', 0x31: 'MBStringLength', 0x32: 'CharToAscii', 0x33: 'AsciiToChar', 0x34: 'GetTime',
    0x35: 'MBStringExtract', 0x36: 'MBCharToAscii', 0x37: 'MBAsciiToChar', 0x3A: 'Delete', 0x3B: 'Delete2',
    0x3C: 'DefineLocal', 0x3D: 'CallFunction', 0x3E: 'Return', 0x3F: 'Modulo', 0x40: 'NewObject',
    0x41: 'DefineLocal2', 0x42: 'InitArray', 0x43: 'InitObject', 0x44: 'TypeOf', 0x45: 'TargetPath',
    0x46: 'Enumerate', 0x47: 'Add2', 0x48: 'Less2', 0x49: 'Equals2', 0x4A: 'ToNumber', 0x4B: 'ToString',
    0x4C: 'PushDuplicate', 0x4D: 'StackSwap', 0x4E: 'GetMember', 0x4F: 'SetMember', 0x50: 'Increment',
    0x51: 'Decrement', 0x52: 'CallMethod', 0x53: 'NewMethod', 0x54: 'InstanceOf', 0x55: 'Enumerate2',
    0x60: 'BitAnd', 0x61: 'BitOr', 0x62: 'BitXor', 0x63: 'BitLShift', 0x64: 'BitRShift', 0x65: 'BitURShift',
    0x66: 'StrictEquals', 0x67: 'Greater', 0x68: 'StringGreater', 0x69: 'Extends',
    0x81: 'GotoFrame', 0x83: 'GetURL', 0x87: 'StoreRegister', 0x88: 'ConstantPool', 0x8A: 'WaitForFrame',
    0x8B: 'SetTarget', 0x8C: 'GotoLabel', 0x8D: 'WaitForFrame2', 0x8E: 'DefineFunction2', 0x8F: 'Try',
    0x94: 'With', 0x96: 'Push', 0x99: 'Jump', 0x9A: 'GetURL2', 0x9B: 'DefineFunction', 0x9D: 'If',
    0x9E: 'Call', 0x9F: 'GotoFrame2',
}


def read_tags(data, off, end):
    while off < end:
        h = struct.unpack_from('<H', data, off)[0]
        code, ln = h >> 6, h & 0x3F
        off += 2
        if ln == 0x3F:
            ln = struct.unpack_from('<I', data, off)[0]
            off += 4
        yield code, data[off:off + ln]
        off += ln
        if code == 0:
            break


def cstr(b, o):
    e = b.index(b'\x00', o)
    return b[o:e].decode('latin-1'), e + 1


def decode_actions(b, o=0, end=None, pool=None, depth=0):
    """Yield (offset, text) lines for the action records in b[o:end]; nested functions are indented."""
    if end is None:
        end = len(b)
    pool = pool if pool is not None else []
    ind = '  ' * depth
    while o < end:
        op = b[o]
        start = o
        o += 1
        if op == 0:
            yield start, ind + 'End'
            break
        body = b''
        if op >= 0x80:
            ln = struct.unpack_from('<H', b, o)[0]
            o += 2
            body = b[o:o + ln]
            o += ln
        name = OPS.get(op, 'op_%02X' % op)
        if op == 0x88:  # ConstantPool
            n = struct.unpack_from('<H', body, 0)[0]
            p = 2
            pool[:] = []
            for _ in range(n):
                s, p = cstr(body, p)
                pool.append(s)
            yield start, ind + 'ConstantPool(%d)' % n
        elif op == 0x96:  # Push
            p = 0
            vals = []
            while p < len(body):
                t = body[p]
                p += 1
                if t == 0:
                    s, p = cstr(body, p); vals.append('"%s"' % s)
                elif t == 1:
                    vals.append('%g' % struct.unpack_from('<f', body, p)[0]); p += 4
                elif t == 2:
                    vals.append('null')
                elif t == 3:
                    vals.append('undefined')
                elif t == 4:
                    vals.append('r%d' % body[p]); p += 1
                elif t == 5:
                    vals.append('true' if body[p] else 'false'); p += 1
                elif t == 6:
                    vals.append('%g' % struct.unpack_from('<d', body, p)[0]); p += 8
                elif t == 7:
                    vals.append('%d' % struct.unpack_from('<i', body, p)[0]); p += 4
                elif t == 8:
                    i = body[p]; p += 1
                    vals.append('"%s"' % pool[i] if i < len(pool) else 'c%d' % i)
                elif t == 9:
                    i = struct.unpack_from('<H', body, p)[0]; p += 2
                    vals.append('"%s"' % pool[i] if i < len(pool) else 'c%d' % i)
                else:
                    vals.append('?%d' % t)
                    break
            yield start, ind + 'Push ' + ', '.join(vals)
        elif op in (0x9B, 0x8E):  # DefineFunction / DefineFunction2
            p = 0
            fname, p = cstr(body, p)
            nparams = struct.unpack_from('<H', body, p)[0]; p += 2
            params = []
            if op == 0x8E:
                nregs = body[p]; p += 1
                flags = struct.unpack_from('<H', body, p)[0]; p += 2
                for _ in range(nparams):
                    r = body[p]; p += 1
                    s, p = cstr(body, p)
                    params.append('r%d=%s' % (r, s))
                head = '%s %s(%s) regs=%d flags=0x%04X' % (name, fname or '<anon>', ', '.join(params), nregs, flags)
            else:
                for _ in range(nparams):
                    s, p = cstr(body, p)
                    params.append(s)
                head = '%s %s(%s)' % (name, fname or '<anon>', ', '.join(params))
            code_len = struct.unpack_from('<H', body, p)[0]
            yield start, ind + head + ' {'
            for line in decode_actions(b, o, o + code_len, list(pool), depth + 1):
                yield line
            yield o + code_len, ind + '}'
            o += code_len
        elif op in (0x99, 0x9D):
            off = struct.unpack_from('<h', body, 0)[0]
            yield start, ind + '%s -> %+d (0x%X)' % (name, off, o + off)
        elif op == 0x87:
            yield start, ind + 'StoreRegister r%d' % body[0]
        elif op == 0x8C:
            yield start, ind + 'GotoLabel "%s"' % cstr(body, 0)[0]
        elif op == 0x81:
            yield start, ind + 'GotoFrame %d' % struct.unpack_from('<H', body, 0)[0]
        elif op == 0x94:
            yield start, ind + 'With len=%d' % struct.unpack_from('<H', body, 0)[0]
        elif op == 0x8B:
            yield start, ind + 'SetTarget "%s"' % cstr(body, 0)[0]
        else:
            yield start, ind + name + ((' [%s]' % body.hex()) if body else '')


def collect_blocks(data, off, end, where):
    for code, body in read_tags(data, off, end):
        if code in (12, 59):
            label = 'DoAction' if code == 12 else 'DoInitAction(sprite %d)' % struct.unpack_from('<H', body, 0)[0]
            yield where + '/' + label, body[2:] if code == 59 else body
        elif code == 39:
            sid = struct.unpack_from('<H', body, 0)[0]
            for blk in collect_blocks(body, 4, len(body), where + '/Sprite%d' % sid):
                yield blk
        elif code == 26:  # PlaceObject2 clip actions
            flags = body[0]
            if flags & 0x80:
                p = 3
                if flags & 0x02: p += 2
                if flags & 0x04: p = skip_matrix(body, p)
                if flags & 0x08: p = skip_cxform(body, p)
                if flags & 0x10: p += 2
                if flags & 0x20: _, p = cstr(body, p)
                if flags & 0x40: p += 2
                p += 2  # reserved
                p += 4  # all-event flags (SWF6+)
                while p + 4 <= len(body):
                    ev = struct.unpack_from('<I', body, p)[0]; p += 4
                    if ev == 0:
                        break
                    ln = struct.unpack_from('<I', body, p)[0]; p += 4
                    blk = body[p:p + ln]
                    if ev & 0x00020000:
                        blk = blk[1:]  # key code
                    yield where + '/ClipEvent(0x%X)' % ev, blk
                    p += ln
        elif code == 40:  # DefineButton2
            aoff = struct.unpack_from('<H', body, 3)[0]
            if aoff:
                p = 3 + aoff
                while p + 4 <= len(body):
                    nxt = struct.unpack_from('<H', body, p)[0]
                    cond = struct.unpack_from('<H', body, p + 2)[0]
                    blk_end = p + nxt if nxt else len(body)
                    yield where + '/Button%d(cond 0x%X)' % (struct.unpack_from('<H', body, 0)[0], cond), body[p + 4:blk_end]
                    if not nxt:
                        break
                    p += nxt


def skip_matrix(b, p):
    bits = b[p] >> 3 & 0  # placeholder; do a proper bit walk
    return _skip_bits(b, p, 'matrix')


def skip_cxform(b, p):
    return _skip_bits(b, p, 'cxform')


def _skip_bits(b, p, kind):
    class R:
        def __init__(s, b, p): s.b, s.pos = b, p * 8
        def bits(s, n):
            v = 0
            for _ in range(n):
                v = (v << 1) | ((s.b[s.pos >> 3] >> (7 - (s.pos & 7))) & 1)
                s.pos += 1
            return v
    r = R(b, p)
    if kind == 'matrix':
        if r.bits(1):
            n = r.bits(5); r.bits(n); r.bits(n)
        if r.bits(1):
            n = r.bits(5); r.bits(n); r.bits(n)
        n = r.bits(5); r.bits(n); r.bits(n)
    else:
        has_add = r.bits(1); has_mul = r.bits(1); n = r.bits(4)
        if has_mul: r.bits(n); r.bits(n); r.bits(n); r.bits(n)
        if has_add: r.bits(n); r.bits(n); r.bits(n); r.bits(n)
    return (r.pos + 7) // 8


def main():
    if len(sys.argv) < 3:
        print(__doc__); return
    data = open(sys.argv[1], 'rb').read()
    assert data[:3] == b'FWS', 'compressed SWF not supported here'
    nbits = data[8] >> 3
    hdr = 8 + (5 + 4 * nbits + 7) // 8 + 4
    blocks = list(collect_blocks(data, hdr, len(data), 'root'))
    args = sys.argv[2:]
    if args == ['--strings']:
        from collections import Counter
        c = Counter()
        for _, blk in blocks:
            for _, line in decode_actions(blk):
                if line.strip().startswith('Push'):
                    for tok in line.split('"')[1::2]:
                        c[tok] += 1
        for s, n in c.most_common():
            print(n, s)
        return
    needles = None if args == ['--all'] else [a.encode('latin-1') for a in args]
    for where, blk in blocks:
        if needles is not None and not any(n in blk for n in needles):
            continue
        lines = list(decode_actions(blk))
        print('==== %s (%d bytes, %d records)' % (where, len(blk), len(lines)))
        for off, line in lines:
            print('%6X  %s' % (off, line))


if __name__ == '__main__':
    main()
