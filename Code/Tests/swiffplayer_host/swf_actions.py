"""SCAFFOLDING (Code/Tests) - root-timeline census + AS2 disassembly for one SWF (2026-09-13). The sibling
swfdump.py reads a SPRITE's display-list tags; this one reads the MAIN timeline: which frames carry labels,
DoAction bodies (size + md5, shared bodies reported) and place/remove tags, and disassembles one frame's
DoAction so a page script's control flow (gotoPage / gotoAndStop / stop) can be read off the file.

  python swf_actions.py <swf> census [first last]     per-frame labels / DoAction (size, md5) / place+remove counts
  python swf_actions.py <swf> actions <frame>         disassemble every DoAction of that root frame (0-based)
"""
import hashlib
import struct
import sys
import zlib


class BitReader:
    """The SWF bit-packed field reader (MATRIX / CXFORM) - the same shape swfdump.py uses."""

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


def load(path):
    data = open(path, "rb").read()
    sig = data[:3]
    if sig == b"CWS":
        data = data[:8] + zlib.decompress(data[8:])
    elif sig != b"FWS":
        raise SystemExit("not a SWF")
    # header: sig(3) ver(1) len(4) RECT(bits) frame_rate(2) frame_count(2)
    p = 8
    nbits = data[p] >> 3
    rect_bits = 5 + 4 * nbits
    p += (rect_bits + 7) // 8
    p += 4
    return data, p


def tags(data, p):
    while p < len(data):
        code_len = struct.unpack_from("<H", data, p)[0]
        code, ln = code_len >> 6, code_len & 0x3F
        p += 2
        if ln == 0x3F:
            ln = struct.unpack_from("<I", data, p)[0]
            p += 4
        yield code, data[p:p + ln]
        p += ln
        if code == 0:
            break


def census(data, p, first, last):
    frame = 0
    labels, actions, places, removes = [], [], 0, 0
    hashes = {}
    rows = []
    for code, body in tags(data, p):
        if code == 43:
            labels.append(body.split(b"\0")[0].decode("latin1"))
        elif code == 12:
            h = hashlib.md5(body).hexdigest()[:10]
            actions.append((len(body), h))
            hashes.setdefault(h, []).append(frame)
        elif code in (4, 26, 70):
            places += 1
        elif code in (5, 28):
            removes += 1
        elif code == 1:
            rows.append((frame, labels, actions, places, removes))
            frame += 1
            labels, actions, places, removes = [], [], 0, 0
    for f, lab, act, pl, rm in rows:
        if f < first or f > last:
            continue
        print("frame %3d  labels=%-22s actions=%-28s place=%d remove=%d" %
              (f, ",".join(lab), " ".join("%d:%s" % a for a in act) or "-", pl, rm))
    dup = {h: fr for h, fr in hashes.items() if len(fr) > 1}
    if dup:
        print("DoAction bodies shared by several frames:")
        for h, fr in dup.items():
            print("  %s frames %s" % (h, fr))
    print("total frames", frame)


NAMES = {0x04: "NextFrame", 0x05: "PrevFrame", 0x06: "Play", 0x07: "Stop", 0x0A: "Add", 0x0B: "Subtract",
         0x0C: "Multiply", 0x0D: "Divide", 0x0E: "Equals", 0x0F: "Less", 0x10: "And", 0x11: "Or", 0x12: "Not",
         0x13: "StringEquals", 0x17: "Pop", 0x1C: "GetVariable", 0x1D: "SetVariable", 0x20: "SetTarget2",
         0x21: "StringAdd", 0x22: "GetProperty", 0x23: "SetProperty", 0x24: "CloneSprite", 0x25: "RemoveSprite",
         0x26: "Trace", 0x2B: "CastOp", 0x2C: "ImplementsOp", 0x30: "RandomNumber", 0x34: "GetTime",
         0x3A: "Delete", 0x3B: "Delete2", 0x3C: "DefineLocal", 0x3D: "CallFunction", 0x3E: "Return",
         0x3F: "Modulo", 0x40: "NewObject", 0x41: "DefineLocal2", 0x42: "InitArray", 0x43: "InitObject",
         0x44: "TypeOf", 0x45: "TargetPath", 0x46: "Enumerate", 0x47: "Add2", 0x48: "Less2", 0x49: "Equals2",
         0x4A: "ToNumber", 0x4B: "ToString", 0x4C: "PushDuplicate", 0x4D: "StackSwap", 0x4E: "GetMember",
         0x4F: "SetMember", 0x50: "Increment", 0x51: "Decrement", 0x52: "CallMethod", 0x53: "NewMethod",
         0x54: "InstanceOf", 0x55: "Enumerate2", 0x60: "BitAnd", 0x61: "BitOr", 0x62: "BitXor",
         0x63: "BitLShift", 0x64: "BitRShift", 0x65: "BitURShift", 0x66: "StrictEquals", 0x67: "Greater",
         0x68: "StringGreater", 0x69: "Extends", 0x81: "GotoFrame", 0x83: "GetURL", 0x87: "StoreRegister",
         0x88: "ConstantPool", 0x8A: "WaitForFrame", 0x8B: "SetTarget", 0x8C: "GoToLabel", 0x8D: "WaitForFrame2",
         0x8E: "DefineFunction2", 0x8F: "Try", 0x94: "With", 0x96: "Push", 0x99: "Jump", 0x9A: "GetURL2",
         0x9B: "DefineFunction", 0x9D: "If", 0x9E: "CallFrame", 0x9F: "GotoFrame2"}


def cstr(b, p):
    e = b.index(b"\0", p)
    return b[p:e].decode("latin1", "replace"), e + 1


def disasm(body, indent=""):
    pool = []
    p = 0
    while p < len(body):
        op = body[p]
        start = p
        p += 1
        if op == 0:
            print("%s%04x  End" % (indent, start))
            break
        ln = 0
        if op >= 0x80:
            ln = struct.unpack_from("<H", body, p)[0]
            p += 2
        arg = body[p:p + ln]
        p += ln
        name = NAMES.get(op, "op_%02x" % op)
        text = ""
        if op == 0x88:
            n = struct.unpack_from("<H", arg, 0)[0]
            q = 2
            pool = []
            for _ in range(n):
                s, q = cstr(arg, q)
                pool.append(s)
            text = "%d strings" % n
        elif op == 0x96:
            items = []
            q = 0
            while q < len(arg):
                t = arg[q]
                q += 1
                if t == 0:
                    s, q = cstr(arg, q)
                    items.append('"%s"' % s)
                elif t == 1:
                    items.append("%g" % struct.unpack_from("<f", arg, q)[0]); q += 4
                elif t == 2:
                    items.append("null")
                elif t == 3:
                    items.append("undefined")
                elif t == 4:
                    items.append("reg%d" % arg[q]); q += 1
                elif t == 5:
                    items.append("true" if arg[q] else "false"); q += 1
                elif t == 6:
                    items.append("%g" % struct.unpack_from("<d", arg, q)[0]); q += 8
                elif t == 7:
                    items.append("%d" % struct.unpack_from("<i", arg, q)[0]); q += 4
                elif t == 8:
                    i = arg[q]; q += 1
                    items.append('"%s"' % pool[i] if i < len(pool) else "pool[%d]" % i)
                elif t == 9:
                    i = struct.unpack_from("<H", arg, q)[0]; q += 2
                    items.append('"%s"' % pool[i] if i < len(pool) else "pool[%d]" % i)
                else:
                    items.append("?type%d" % t)
                    break
            text = ", ".join(items)
        elif op in (0x99, 0x9D):
            off = struct.unpack_from("<h", arg, 0)[0]
            text = "-> %04x" % (p + off)
        elif op == 0x81:
            text = "frame %d" % struct.unpack_from("<H", arg, 0)[0]
        elif op == 0x8C:
            text = '"%s"' % cstr(arg, 0)[0]
        elif op == 0x9F:
            flags = arg[0]
            text = "play=%d bias=%d" % (flags & 1, (flags >> 1) & 1)
        elif op == 0x87:
            text = "reg%d" % arg[0]
        elif op == 0x9B:
            nm, q = cstr(arg, 0)
            n = struct.unpack_from("<H", arg, q)[0]
            q += 2
            params = []
            for _ in range(n):
                s, q = cstr(arg, q)
                params.append(s)
            size = struct.unpack_from("<H", arg, q)[0]
            text = 'function "%s"(%s) body %d bytes -> %04x' % (nm, ", ".join(params), size, p + size)
        elif op == 0x8E:
            nm, q = cstr(arg, 0)
            n = struct.unpack_from("<H", arg, q)[0]
            regs = arg[q + 2]
            flags = struct.unpack_from("<H", arg, q + 3)[0]
            q += 5
            params = []
            for _ in range(n):
                r = arg[q]
                s, q = cstr(arg, q + 1)
                params.append("r%d:%s" % (r, s) if r else s)
            size = struct.unpack_from("<H", arg, q)[0]
            text = 'function2 "%s"(%s) regs=%d flags=%04x body %d bytes -> %04x' % (
                nm, ", ".join(params), regs, flags, size, p + size)
        elif op == 0x8B:
            text = '"%s"' % cstr(arg, 0)[0]
        print("%s%04x  %-16s %s" % (indent, start, name, text))


def main():
    data, p = load(sys.argv[1])
    mode = sys.argv[2]
    if mode == "census":
        first = int(sys.argv[3]) if len(sys.argv) > 3 else 0
        last = int(sys.argv[4]) if len(sys.argv) > 4 else 10 ** 6
        census(data, p, first, last)
    elif mode == "actions":
        want = int(sys.argv[3])
        frame = 0
        for code, body in tags(data, p):
            if code == 12 and frame == want:
                print("== DoAction at root frame %d, %d bytes" % (frame, len(body)))
                disasm(body)
            elif code == 1:
                frame += 1
                if frame > want:
                    break
    elif mode == "inits":
        # DoInitAction (59): UI16 sprite id + the class-definition actions Flash MX 2004 emits per
        # exported symbol. An optional substring filters on the raw body (constant-pool strings).
        needle = sys.argv[3].encode("latin1") if len(sys.argv) > 3 else None
        for code, body in tags(data, p):
            if code == 59:
                sid = struct.unpack_from("<H", body, 0)[0]
                if needle is None or needle in body:
                    print("DoInitAction sprite %-5d %6d bytes  md5 %s" %
                          (sid, len(body) - 2, hashlib.md5(body).hexdigest()[:10]))
    elif mode == "init":
        want = int(sys.argv[3])
        for code, body in tags(data, p):
            if code == 59 and struct.unpack_from("<H", body, 0)[0] == want:
                print("== DoInitAction sprite %d, %d bytes" % (want, len(body) - 2))
                disasm(body[2:])
    elif mode == "place":
        # PlaceObject2 (26) tags of one DefineSprite (or the root with id 0), with their ClipActions:
        # depth, character, name and every clip-event handler (flags + disassembly). Flash MX 2004
        # stores a component instance's parameters in its onClipEvent(initialize) handler.
        want = int(sys.argv[3])
        def dump_places(stream, start):
            frame = 0
            for c2, b2 in tags(stream, start):
                if c2 == 1:
                    frame += 1
                elif c2 == 26:
                    flags = b2[0]
                    q = 1
                    depth = struct.unpack_from("<H", b2, q)[0]; q += 2
                    cid = None
                    if flags & 0x02:
                        cid = struct.unpack_from("<H", b2, q)[0]; q += 2
                    # matrix / cxform / ratio / name / clipdepth are bit-packed; only walk what we need
                    if flags & 0x04:
                        r = BitReader(b2, q)
                        if r.u(1): n = r.u(5); r.s(n); r.s(n)
                        if r.u(1): n = r.u(5); r.s(n); r.s(n)
                        n = r.u(5); r.s(n); r.s(n)
                        r.align(); q = r.p
                    if flags & 0x08:
                        r = BitReader(b2, q)
                        has_add = r.u(1); has_mul = r.u(1); n = r.u(4)
                        cnt = (4 if has_mul else 0) + (4 if has_add else 0)
                        for _ in range(cnt): r.s(n)
                        r.align(); q = r.p
                    if flags & 0x10:
                        q += 2
                    name = ""
                    if flags & 0x20:
                        name, q = cstr(b2, q)
                    if flags & 0x40:
                        q += 2
                    print("frame %d  depth %d  id %s  name '%s'  flags %02x" % (frame, depth, cid, name, flags))
                    if flags & 0x80:
                        q += 2                                   # reserved
                        allflags = struct.unpack_from("<I", b2, q)[0]; q += 4
                        while True:
                            ev = struct.unpack_from("<I", b2, q)[0]; q += 4
                            if ev == 0:
                                break
                            size = struct.unpack_from("<I", b2, q)[0]; q += 4
                            body = b2[q:q + size]; q += size
                            print("  clip event %08x (%d bytes)" % (ev, size))
                            disasm(body, "     ")
        if want == 0:
            dump_places(data, p)
        else:
            for code, body in tags(data, p):
                if code == 39 and struct.unpack_from("<H", body, 0)[0] == want:
                    print("== DefineSprite %d" % want)
                    dump_places(body, 4)
    elif mode == "sprite":
        # DefineSprite (39): UI16 id, UI16 frame count, then the sprite's own tag stream. Prints
        # the per-frame labels and disassembles every DoAction inside it.
        want = int(sys.argv[3])
        for code, body in tags(data, p):
            if code == 39 and struct.unpack_from("<H", body, 0)[0] == want:
                frames = struct.unpack_from("<H", body, 2)[0]
                print("== DefineSprite %d, %d frames, %d bytes" % (want, frames, len(body) - 4))
                frame = 0
                for c2, b2 in tags(body, 4):
                    if c2 == 43:
                        print("frame %d label %s" % (frame, b2.split(b"\0")[0].decode("latin1")))
                    elif c2 == 12:
                        print("-- DoAction at sprite frame %d, %d bytes" % (frame, len(b2)))
                        disasm(b2, "   ")
                    elif c2 == 1:
                        frame += 1


if __name__ == "__main__":
    main()
