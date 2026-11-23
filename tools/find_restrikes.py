#!/usr/bin/env python3
"""Find MIDI files with pedalled restrikes on the grand piano.

A restrike: a key of a piano channel (General MIDI program 1, not the drum channel) struck again
while the sustain pedal has stayed down ever since its previous stroke, so that its strings are
still ringing when the hammer lands on them.

    tools/find_restrikes.py                    search ~/midi and ~/Music/midi
    tools/find_restrikes.py DIR_OR_FILE ...    search there
    tools/find_restrikes.py --top 40 --min-gap 0.1 --max-gap 1.5

Ranked by restrikes; for each file its densest stretch and a line to play from just before it.
"""
import argparse
import bisect
import collections
import multiprocessing
import os
import shlex
import struct
import sys

PITCH = 'C C# D Eb E F F# G Ab A Bb B'.split()


def note_name(key):
    return f'{PITCH[key % 12]}{key // 12 - 1}'


def clock(seconds):
    s = int(seconds)
    return f'{s // 60}:{s % 60:02d}'


def read_events(path):
    """(tick, order, kind, channel, a, b) of every track, and the tempo map's ticks and seconds."""
    with open(path, 'rb') as fh:
        data = fh.read()
    if data[:4] != b'MThd' or len(data) < 14:
        raise ValueError('not a MIDI file')
    _, tracks, division = struct.unpack('>HHH', data[8:14])
    if division & 0x8000 or division == 0:
        raise ValueError('SMPTE time is not handled')
    pos = 8 + struct.unpack('>I', data[4:8])[0]
    events, tempos = [], []

    for _ in range(tracks):
        if pos + 8 > len(data):
            break
        size = struct.unpack('>I', data[pos + 4:pos + 8])[0]
        chunk_is_track = data[pos:pos + 4] == b'MTrk'
        body = data[pos + 8:pos + 8 + size]
        pos += 8 + size
        if not chunk_is_track:
            continue
        i, tick, status = 0, 0, 0

        def varlen():
            nonlocal i
            value = 0
            for _ in range(4):
                byte = body[i]
                i += 1
                value = value << 7 | byte & 127
                if byte < 128:
                    break
            return value

        try:
            while i < len(body):
                tick += varlen()
                byte = body[i]
                if byte >= 128:
                    i += 1
                    if byte < 0xf0:
                        status = byte
                else:
                    byte = status
                if byte == 0xff:
                    kind = body[i]
                    i += 1
                    length = varlen()
                    if kind == 0x51 and length == 3:
                        tempos.append((tick, int.from_bytes(body[i:i + 3], 'big')))
                    i += length
                elif byte in (0xf0, 0xf7):
                    i += varlen()
                elif byte & 0xf0 in (0xc0, 0xd0):
                    if byte & 0xf0 == 0xc0:
                        events.append((tick, 0, 'program', byte & 15, body[i], 0))
                    i += 1
                elif byte >= 0x80:
                    a, b = body[i], body[i + 1]
                    i += 2
                    high = byte & 0xf0
                    if high == 0xb0 and a == 64:
                        events.append((tick, 1, 'pedal', byte & 15, b, 0))
                    elif high == 0x90 and b > 0:
                        events.append((tick, 3, 'on', byte & 15, a, b))
                else:
                    break  # (a damaged track: what was read of it stands)
        except IndexError:
            pass

    events.sort(key=lambda e: e[:2])
    tempos.sort()
    if not tempos or tempos[0][0] > 0:
        tempos.insert(0, (0, 500000))
    ticks, seconds = [], []
    now = 0.0
    last_tick, last_tempo = tempos[0]
    for t, tempo in tempos:
        now += (t - last_tick) * last_tempo / division / 1e6
        ticks.append(t)
        seconds.append((now, tempo))
        last_tick, last_tempo = t, tempo

    def to_seconds(tick):
        j = bisect.bisect_right(ticks, tick) - 1
        return seconds[j][0] + (tick - ticks[j]) * seconds[j][1] / division / 1e6

    return events, to_seconds


def scan(job):
    path, min_gap, max_gap = job
    try:
        events, to_seconds = read_events(path)
    except (OSError, ValueError, struct.error):
        return None
    program = collections.defaultdict(int)
    pedal = collections.defaultdict(bool)
    pedal_since = collections.defaultdict(lambda: -1.0)  # (when it last went down)
    last_on = {}
    hits = []
    end = 0.0
    for tick, _, kind, channel, a, b in events:
        t = to_seconds(tick)
        end = max(end, t)
        if kind == 'program':
            program[channel] = a
        elif kind == 'pedal':
            down = a >= 64
            if down and not pedal[channel]:
                pedal_since[channel] = t
            pedal[channel] = down
        elif channel != 9 and program[channel] == 0:
            before = last_on.get((channel, a))
            if before is not None and pedal[channel] and pedal_since[channel] <= before:
                gap = t - before
                if min_gap <= gap <= max_gap:
                    hits.append((t, a, b, gap))
            last_on[(channel, a)] = t
    if not hits:
        return None

    times = [h[0] for h in hits]
    best_at, best_n = times[0], 0
    for j, t in enumerate(times):  # (the 10 s with the most of them)
        n = bisect.bisect_right(times, t + 10) - j
        if n > best_n:
            best_at, best_n = t, n
    keys = collections.Counter(h[1] for h in hits).most_common(3)
    gaps = sorted(h[3] for h in hits)
    return dict(path=path, count=len(hits), minutes=max(end, 1) / 60, best_at=best_at, best_n=best_n,
                keys=keys, median_gap=gaps[len(gaps) // 2])


def midi_files(roots):
    seen = set()
    for root in roots:
        root = os.path.expanduser(root)
        if os.path.isfile(root):
            walk = [(os.path.dirname(root), [], [os.path.basename(root)])]
        else:
            walk = os.walk(root)
        for folder, _, names in walk:
            for name in names:
                if name.lower().endswith(('.mid', '.midi', '.kar')):
                    path = os.path.realpath(os.path.join(folder, name))
                    if path not in seen:
                        seen.add(path)
                        yield path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('roots', nargs='*', default=['~/midi', '~/Music/midi'])
    ap.add_argument('--top', type=int, default=25, help='files to list (default 25)')
    ap.add_argument('--min-gap', type=float, default=0.06, help='shortest time between the strokes, s')
    ap.add_argument('--max-gap', type=float, default=3.0, help='longest time between the strokes, s')
    args = ap.parse_args()

    files = list(midi_files(args.roots))
    with multiprocessing.Pool() as pool:
        found = [r for r in pool.imap_unordered(scan, [(f, args.min_gap, args.max_gap) for f in files], 16) if r]
    found.sort(key=lambda r: (-r['count'], r['path']))
    print(f'{len(files)} MIDI files, {len(found)} with pedalled restrikes on the grand piano')
    for r in found[:args.top]:
        keys = ' '.join(f'{note_name(k)}x{n}' for k, n in r['keys'])
        print(f"\n{r['count']:5d} restrikes ({r['count'] / r['minutes']:.0f} a minute, median {r['median_gap'] * 1000:.0f} ms apart), "
              f"most at {clock(r['best_at'])} ({r['best_n']} in 10 s); keys {keys}")
        print(f"      ./pajupilli -start_time {clock(max(r['best_at'] - 2, 0))} -play {shlex.quote(r['path'])}")


if __name__ == '__main__':
    sys.exit(main())
