"""SCAFFOLDING - diff two [swiff]/[stream] tracer logs.

usage: swiffdiff.py A.log B.log [displayNo] [--tag swiff|stream] [--summary]
  Strips the 'HH:MM:SS.mmm [tid] ' prefix, extracts one display bracket from each log and prints a
  unified diff. --summary prints the non-bracket lines (loadTexture/setScale/setMovie*/createFont)
  and the per-display summary lines of each log instead.
"""
import re
import sys
import difflib

PREFIX = re.compile(r'^\d\d:\d\d:\d\d\.\d{3} \[\s*\d+\] (.*)$')


def load(path):
    out = []
    with open(path, encoding='utf-8', errors='replace') as f:
        for raw in f:
            raw = raw.rstrip('\n')
            m = PREFIX.match(raw)
            out.append(m.group(1) if m else raw)
    return out


def bracket(lines, n, tag):
    if tag == 'swiff':
        begin, end = r'\[swiff\] beginDisplay #%d\b' % n, r'\[swiff\] endDisplay #%d\b' % n
    else:
        begin, end = r'\[stream\] display #%d\b' % n, r'\[stream\] display #%d done' % n
    out, on = [], False
    for l in lines:
        if not on and re.search(begin, l):
            on = True
        if on:
            out.append(l)
        if on and re.search(end, l):
            break
    return out


def summary(lines, tag):
    key = r'\[%s\]' % tag
    for l in lines:
        if not re.search(key, l):
            continue
        if re.search(r'beginDisplay #|endDisplay #|display #', l) or not l.startswith('[' + tag + ']  '):
            if re.search(r'\[%s\]   ' % tag, l):
                continue
            print(l)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    opts = [a for a in sys.argv[1:] if a.startswith('--')]
    tag = 'swiff'
    for o in opts:
        if o.startswith('--tag'):
            tag = o.split('=', 1)[1] if '=' in o else 'swiff'
    a, b = args[0], args[1]
    n = int(args[2]) if len(args) > 2 else 1
    A, B = load(a), load(b)
    if '--summary' in opts:
        print('=== %s ===' % a); summary(A, tag)
        print('=== %s ===' % b); summary(B, tag)
        return
    BA, BB = bracket(A, n, tag), bracket(B, n, tag)
    print('--- %s  display #%d: %d lines' % (a, n, len(BA)))
    print('+++ %s  display #%d: %d lines' % (b, n, len(BB)))
    same = 0
    for l in difflib.unified_diff(BA, BB, lineterm='', n=2):
        print(l)
    if BA == BB:
        print('(identical)')


if __name__ == '__main__':
    main()
