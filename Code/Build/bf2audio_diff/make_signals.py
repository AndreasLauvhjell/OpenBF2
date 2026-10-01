"""[TEMP] SCAFFOLDING - writes the test signals the diff host plays: tone wavs of several formats,
two with loop points (cue+plst and cue+LIST/adtl/ltxt), one with a stray LIST INFO chunk, and an
EAX 3.0 reverb preset in the line format Reverb::load reads.  usage: make_signals.py <out dir>"""
import math, os, struct, sys

out = sys.argv[1]
os.makedirs(out, exist_ok=True)

def tone(rate, seconds, freq, channels=1, freq2=None):
    n = int(rate * seconds)
    frames = []
    for i in range(n):
        t = i / rate
        a = int(12000 * math.sin(2 * math.pi * freq * t))
        if channels == 1:
            frames.append(struct.pack('<h', a))
        else:
            b = int(12000 * math.sin(2 * math.pi * (freq2 or freq * 1.5) * t))
            frames.append(struct.pack('<hh', a, b))
    return b''.join(frames), n

def chunk(cid, payload):
    if len(payload) & 1:
        payload += b'\0'
    return cid + struct.pack('<I', len(payload) - (1 if False else 0)) + payload

def wav(path, rate, channels, data, extra_before=(), extra_after=()):
    fmt = struct.pack('<HHIIHH', 1, channels, rate, rate * 2 * channels, 2 * channels, 16)
    body = b'WAVE' + chunk(b'fmt ', fmt)
    for c in extra_before:
        body += c
    body += chunk(b'data', data)
    for c in extra_after:
        body += c
    with open(path, 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', len(body)) + body)

def cue(sample_offset, name=1):
    point = struct.pack('<IIIIII', name, sample_offset, 0x61746164, 0, 0, sample_offset)   # 'data'
    return chunk(b'cue ', struct.pack('<I', 1) + point)                                     # 28 bytes

def plst(length, name=1):
    return chunk(b'plst', struct.pack('<I', 1) + struct.pack('<III', name, length, 1))    # 16 bytes

def adtl_ltxt(sample_length, name=1):
    ltxt = struct.pack('<II', name, sample_length) + b'rgn ' + struct.pack('<HHHH', 0, 0, 0, 0)
    return chunk(b'LIST', b'adtl' + chunk(b'ltxt', ltxt))

def list_info():
    return chunk(b'LIST', b'INFO' + chunk(b'ISFT', b'bf2audio-diff\0'))

# Durations are chosen to END MID-FRAME at the host's 30 fps (N.5 frames), so that both runs see
# the source stop in the same frame.
# 1. mono 22050, 0.45 s (13.5 frames)
d, n = tone(22050, 0.45, 440.0)
wav(os.path.join(out, 'mono22k.wav'), 22050, 1, d)
# 2. stereo 44100, 1.0167 s (30.5 frames)
d, n = tone(44100, 1.0167, 330.0, 2, 495.0)
wav(os.path.join(out, 'stereo44k.wav'), 44100, 2, d)
# 3. mono 44100 with cue + plst loop points: intro 0.2 s, loop 0.4 s, outro 0.35 s (28.5 frames)
rate = 44100
intro = int(rate * 0.2); loop = int(rate * 0.4); outro = int(rate * 0.35)
d1, _ = tone(rate, 0.2, 220.0); d2, _ = tone(rate, 0.4, 440.0); d3, _ = tone(rate, 0.35, 880.0)
wav(os.path.join(out, 'loop_plst.wav'), rate, 1, d1 + d2 + d3, extra_before=(cue(intro), plst(loop - 1)))
# 4. the same with a LIST/adtl/ltxt loop block instead of the playlist
wav(os.path.join(out, 'loop_ltxt.wav'), rate, 1, d1 + d2 + d3, extra_before=(cue(intro), adtl_ltxt(loop)))
# 5. mono 11025 with a LIST INFO chunk after the data (must be skipped), 0.4167 s (12.5 frames)
d, n = tone(11025, 0.4167, 660.0)
wav(os.path.join(out, 'mono11k_info.wav'), 11025, 1, d, extra_after=(list_info(),))
# 6. mono 44100, 2.0167 s (60.5 frames) - long enough to be cut off
d, n = tone(44100, 2.0167, 262.0)
wav(os.path.join(out, 'mono44k_long.wav'), 44100, 1, d)

# The EAX 3.0 preset, in Reverb::load's line order: two header lines, the environment value,
# then (label, value) pairs, the two pans as (label, x, y, z).
lines = ['bf2audio-diff reverb preset', 'Environment', '26',
         'EnvironmentSize', '7.5', 'EnvironmentDiffusion', '0.8', 'Room', '-1000', 'RoomHF', '-1200', 'RoomLF', '-100',
         'DecayTime', '1.49', 'DecayHFRatio', '0.83', 'DecayLFRatio', '1.0', 'Reflections', '-2602', 'ReflectionsDelay', '0.007',
         'ReflectionsPan', '0', '0.1', '0.2', 'Reverb', '200', 'ReverbDelay', '0.011', 'ReverbPan', '0.3', '0', '0.4',
         'EchoTime', '0.25', 'EchoDepth', '0', 'ModulationTime', '0.25', 'ModulationDepth', '0', 'AirAbsorptionHF', '-5',
         'HFReference', '5000', 'LFReference', '250', 'RoomRolloffFactor', '0', 'Flags', '63']
with open(os.path.join(out, 'preset.rev'), 'w', newline='\n') as f:
    f.write('\n'.join(lines) + '\n')
print("signals written to", out)
