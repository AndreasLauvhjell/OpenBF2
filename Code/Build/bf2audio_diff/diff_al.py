"""[TEMP] SCAFFOLDING - compares two logs (the shim's AL log, or the host's stdout) as SEQUENCES with a
numeric tolerance, and sorts every hunk into TIMING or BEHAVIOUR:
  timing    - a block that merely moved across a frame marker (the same calls, one frame later),
              or an extra/missing per-frame refresh of a still-playing source (state poll, gain,
              pitch, position, velocity, stream refill) - wrap_oal reports a source's end and its
              processed buffers on its own clock, so these land one poll apart between runs;
  behaviour - anything else: a different value, a different call, a different order.
Only behaviour hunks fail the comparison. x87-versus-SSE rounding is absorbed by the tolerance.
usage: diff_al.py <retail log> <rebuilt log> [max hunks] [significant digits] [nofnv]
  nofnv: ignore the PCM hashes of alBufferData (streamed Ogg: two libvorbis generations decode to
  slightly different samples, which is the decoder's business, not the engine's)."""
import difflib, re, sys

a_path, b_path = sys.argv[1], sys.argv[2]
max_show = int(sys.argv[3]) if len(sys.argv) > 3 else 12
digits = int(sys.argv[4]) if len(sys.argv) > 4 else 4
nofnv = len(sys.argv) > 5 and sys.argv[5] == 'nofnv'
NUM = re.compile(r'-?\d+\.\d+(?:[eE][-+]?\d+)?|-?\d+[eE][-+]?\d+')
FNV = re.compile(r'fnv=[0-9A-F]{8}')
FRAME = re.compile(r'^== frame \d+$')
NEUTRAL = [re.compile(p) for p in (
    r'^== frame \d+$',
    r'^alListener3f\(0x1006, ',                       # the per-frame listener velocity
    r'^alGetSourcei\(s\d+, 0x1010\) -> 411[46]$',      # state poll: playing / stopped
    r'^alGetSourcei\(s\d+, 0x101[56]\) -> \d+$',       # buffers queued / processed
    r'^alGetError\(\) -> 0x0$',
    r'^alSourcef\(s\d+, 0x100A, ',                     # gain
    r'^alSourcef\(s\d+, 0x1003, ',                     # pitch
    r'^alSourcef\(s\d+, 0x10(20|21|22|01|02), ',       # interactive mode re-applies distance and cone each frame
    r'^alSourcefv\(s\d+, 0x100[456], ',                # position / direction / velocity
    r'^alSourceUnqueueBuffers\(s\d+, 1\)',             # stream refill
    r'^alBufferData\(b\d+, 0x110[13], size=(2048|8192), ',
    r'^alSourceQueueBuffers\(s\d+, 1\)',
    r'^EAXSet\(buffer, ALL, s\d+\) ',                  # the per-frame EAX buffer block under EAX
    r'^  -> 0x0$',                                     # its result line
    r'^\s+\[f\d+\] playing=',                          # the host's per-frame stats line
)]

def load(p):
    lines = []
    for l in open(p, encoding='utf-8', errors='replace'):
        l = l.rstrip('\n')
        if l.startswith('# alshim') or l.startswith('Memory.dll='):
            continue
        lines.append(l)
    return lines

def norm(l):
    def r(m):
        v = float(m.group(0))
        if abs(v) < 1e-6:
            return '0'
        return ('%%.%dg' % digits) % v
    if nofnv:
        l = FNV.sub('fnv=*', l)
    return NUM.sub(r, l)

def strip_markers(lines):
    return [l for l in lines if not FRAME.match(l)]

def neutral(lines):
    return all(any(p.match(l) for p in NEUTRAL) for l in lines)

a, b = load(a_path), load(b_path)
na, nb = [norm(l) for l in a], [norm(l) for l in b]
sm = difflib.SequenceMatcher(None, na, nb, autojunk=False)
hunks = [op for op in sm.get_opcodes() if op[0] != 'equal']

# Each hunk's CORE is what is left after the frame markers and the per-frame refresh / poll /
# refill lines are removed. A hunk is timing when its two cores agree (only refreshes differ), or
# when another hunk carries the same cores swapped (a block - a sample's end, a stream's stop -
# that landed one frame later on one side).
def core(lines):
    return [l for l in strip_markers(lines) if not any(p.match(l) for p in NEUTRAL)]
content = []
for tag, i1, i2, j1, j2 in hunks:
    content.append((core(na[i1:i2]), core(nb[j1:j2])))
timing = [False] * len(hunks)
for x in range(len(hunks)):
    left, right = content[x]
    if left == right:
        timing[x] = True
        continue
    for y in range(len(hunks)):
        if y != x and not timing[y] and content[y] == (right, left):
            timing[x] = timing[y] = True
            break
behaviour = [h for h, t in zip(hunks, timing) if not t]
print("%s: %d lines | %s: %d lines | hunks: %d (timing: %d, behaviour: %d)"
      % (a_path, len(a), b_path, len(b), len(hunks), len(hunks) - len(behaviour), len(behaviour)))
for tag, i1, i2, j1, j2 in behaviour[:max_show]:
    print("  hunk %s retail[%d:%d] rebuilt[%d:%d]  (context: %s)" % (tag, i1 + 1, i2, j1 + 1, j2, (na[i1 - 1] if i1 > 0 else '')[:80]))
    for l in a[i1:min(i2, i1 + 8)]:
        print("    - %s" % l[:200])
    if i2 - i1 > 8:
        print("    - ... %d more" % (i2 - i1 - 8))
    for l in b[j1:min(j2, j1 + 8)]:
        print("    + %s" % l[:200])
    if j2 - j1 > 8:
        print("    + ... %d more" % (j2 - j1 - 8))
sys.exit(1 if behaviour else 0)
