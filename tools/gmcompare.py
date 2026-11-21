#!/usr/bin/env python3
"""pajupilli against several SoundFonts, one General MIDI instrument at a time
(research/gm_compare.md).

    tools/gmcompare.py run --bin ./pajupilli --programs 0-127,drums [--jobs 8]
    tools/gmcompare.py run --bin /path/to/pajupilli --programs 40 --force   # re-run one
    tools/gmcompare.py report                                             # rebuild the report

Each instrument plays tools/synthcompare.py's profile probe (keys a few semitones apart over the
GM program's range, velocities 40/64/96/127, each held 2 s in a 4 s slot) and a short phrase (six
legato notes, four staccato notes) on:
  - pajupilli (-director off), the binary given by --bin;
  - TiMidity++ with FluidR3_GM, TimGM6mb and Jnsgm2 (one cfg per SoundFont that maps every
    program and drum key again: a bare `soundfont` line leaves Debian's FluidR3 mapping in force);
  - FluidSynth (libfluidsynth through ctypes, reverb and chorus off) with SGM-V2.01 (an SF3, which
    TiMidity plays as noise);
  - FluidSynth with FluidR3_GM, TimGM6mb and Jnsgm2 as a check only (not a reference): it plays
    the SF2 filter and modulation envelopes that TiMidity ignores, so a large TiMidity-FluidSynth
    gap for a font marks an instrument where TiMidity misrepresents that font.
The four references (three TiMidity, one FluidSynth) form the "reference cloud": a difference is
scored by how far pajupilli lies from the cloud's median in units of the larger of the measure's
JND and half the cloud's range, and whether it lies outside the cloud.

Per instrument the results go to research/gm_compare/data/<key>.json (numbers only, no audio); the
renders are deleted when the instrument is done. `report` reads every JSON there and writes
research/gm_compare.md, merging research/gm_compare/notes.md (hand-written verdicts and cross
references, one `## <key>` section per instrument).
"""
import argparse, ctypes, hashlib, json, math, os, re, shutil, subprocess, sys, time, traceback
from concurrent.futures import ProcessPoolExecutor, as_completed

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
for v in ('OPENBLAS_NUM_THREADS', 'OMP_NUM_THREADS', 'MKL_NUM_THREADS'): os.environ.setdefault(v, '1')
import numpy as np
from scipy.io import wavfile
import synthcompare as sc

FS = sc.FS
_harmonic_f0 = sc.harmonic_f0


def _safe_harmonic_f0(m, fe, harmonics=8):
    """synthcompare's f0, but its parabolic refinement can run away on a near-silent or inharmonic
    frame (even below zero): then the expected pitch."""
    f0, f, P = _harmonic_f0(m, fe, harmonics)
    if not (fe * 0.95 < f0 < fe * 1.05): f0 = fe
    return f0, f, P


sc.harmonic_f0 = _safe_harmonic_f0
GM = list(sc.GM) + ('Lead 1 (square),Lead 2 (sawtooth),Lead 3 (calliope),Lead 4 (chiff),Lead 5 (charang),Lead 6 (voice),'
                    'Lead 7 (fifths),Lead 8 (bass + lead),Pad 1 (new age),Pad 2 (warm),Pad 3 (polysynth),Pad 4 (choir),'
                    'Pad 5 (bowed),Pad 6 (metallic),Pad 7 (halo),Pad 8 (sweep),FX 1 (rain),FX 2 (soundtrack),FX 3 (crystal),'
                    'FX 4 (atmosphere),FX 5 (brightness),FX 6 (goblins),FX 7 (echoes),FX 8 (sci-fi),Sitar,Banjo,Shamisen,Koto,'
                    'Kalimba,Bagpipe,Fiddle,Shanai,Tinkle Bell,Agogo,Steel Drums,Woodblock,Taiko Drum,Melodic Tom,Synth Drum,'
                    'Reverse Cymbal,Guitar Fret Noise,Breath Noise,Seashore,Bird Tweet,Telephone Ring,Helicopter,Applause,'
                    'Gunshot').split(',')[len(sc.GM) - 80:]
assert len(GM) == 128, len(GM)
DATA = os.path.join(ROOT, 'research', 'gm_compare', 'data')
NOTES = os.path.join(ROOT, 'research', 'gm_compare', 'notes.md')
REPORT = os.path.join(ROOT, 'research', 'gm_compare.md')
SF = dict(FluidR3='/usr/share/sounds/sf2/FluidR3_GM.sf2', TimGM6mb='/usr/share/sounds/sf2/TimGM6mb.sf2',
          Jnsgm2=os.path.expanduser('~/soundfonts/Jnsgm2.sf2'), SGM=os.path.expanduser('~/soundfonts/SGM-V2.01.sf3'))
# name -> (renderer kind, font); the first four are the references, the rest the TiMidity check
SYNTHS = [('pajupilli', 'pj', None),
          ('FluidR3', 'tim', 'FluidR3'), ('TimGM6mb', 'tim', 'TimGM6mb'), ('Jnsgm2', 'tim', 'Jnsgm2'), ('SGM', 'fs', 'SGM'),
          ('FluidR3/fs', 'fs', 'FluidR3'), ('TimGM6mb/fs', 'fs', 'TimGM6mb'), ('Jnsgm2/fs', 'fs', 'Jnsgm2')]
REFS = ['FluidR3', 'TimGM6mb', 'Jnsgm2', 'SGM']
CHECK = [('FluidR3', 'FluidR3/fs'), ('TimGM6mb', 'TimGM6mb/fs'), ('Jnsgm2', 'Jnsgm2/fs')]


def pkey(p):
    return 'drums' if p == 'drums' else 'p%03d' % p


# ---------------------------------------------------------------------------------------------
# FluidSynth through ctypes

_FL = None
_FS_SYNTHS = {}


def _fl():
    global _FL
    if _FL is None:
        L = ctypes.CDLL('libfluidsynth.so.3')
        L.new_fluid_settings.restype = ctypes.c_void_p
        L.new_fluid_synth.restype = ctypes.c_void_p; L.new_fluid_synth.argtypes = [ctypes.c_void_p]
        L.fluid_settings_setnum.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_double]
        L.fluid_settings_setint.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        L.fluid_synth_sfload.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        for f in ('fluid_synth_noteon', 'fluid_synth_cc'):
            getattr(L, f).argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        L.fluid_synth_noteoff.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        L.fluid_synth_program_change.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        L.fluid_synth_system_reset.argtypes = [ctypes.c_void_p]
        L.fluid_synth_write_float.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                                              ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        L.fluid_set_log_function.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p]
        for lvl in range(5): L.fluid_set_log_function(lvl, None, None)
        _FL = L
    return _FL


def _fs_synth(font):
    if font not in _FS_SYNTHS:
        L = _fl(); s = L.new_fluid_settings()
        L.fluid_settings_setnum(s, b'synth.sample-rate', float(FS))
        L.fluid_settings_setint(s, b'synth.reverb.active', 0); L.fluid_settings_setint(s, b'synth.chorus.active', 0)
        L.fluid_settings_setint(s, b'synth.polyphony', 256)
        L.fluid_settings_setint(s, b'synth.cpu-cores', 1)
        syn = L.new_fluid_synth(s)
        if L.fluid_synth_sfload(syn, SF[font].encode(), 1) < 0: raise SystemExit('FluidSynth could not load ' + SF[font])
        _FS_SYNTHS[font] = syn
    return _FS_SYNTHS[font]


def render_fluid(font, mid, wav):
    L = _fl(); syn = _fs_synth(font); L.fluid_synth_system_reset(syn)
    fmt, div, tracks = sc.read_smf(mid); t2s = sc.tempo_map(div, tracks)
    ev = sorted((t2s(t), k, s, d) for tr in tracks for t, k, s, d in tr)
    end = max(e[0] for e in ev) + 0.5
    out = np.zeros((int(end * FS) + 1, 2), np.float32); pos = 0

    def run_to(n):
        nonlocal pos
        while pos < n:
            m = min(n - pos, 4096)
            l = np.zeros(m, np.float32); r = np.zeros(m, np.float32)
            L.fluid_synth_write_float(syn, m, l.ctypes.data, 0, 1, r.ctypes.data, 0, 1)
            out[pos:pos + m, 0] = l; out[pos:pos + m, 1] = r; pos += m
    for t, kind, s, d in ev:
        run_to(int(round(t * FS)))
        if kind != 'ch': continue
        ch, hi = s & 15, s & 0xF0
        if hi == 0x90 and d[1] > 0: L.fluid_synth_noteon(syn, ch, d[0], d[1])
        elif hi == 0x80 or hi == 0x90: L.fluid_synth_noteoff(syn, ch, d[0])
        elif hi == 0xC0: L.fluid_synth_program_change(syn, ch, d[0])
        elif hi == 0xB0: L.fluid_synth_cc(syn, ch, d[0], d[1])
    run_to(len(out))
    wavfile.write(wav, FS, out)


# ---------------------------------------------------------------------------------------------
# Rendering one probe on one synth


def render(name, kind, font, mid, work, pjbin):
    wav = os.path.join(work, '%s_%s.wav' % (os.path.basename(mid)[:-4], name.replace('/', '_')))
    log = ''
    if not os.path.exists(wav):
        if kind == 'pj':
            env = dict(os.environ, PJ_PLAY_VELMEDIAN='1000', PJ_THREADS='1')
            cmd = [pjbin, '-director', 'off', '-play', mid, '-o', wav + '.tmp.wav']
        elif kind == 'tim':
            sc.CACHE = work
            cmd = ['timidity', '-c', sc.timidity_cfg(SF[font]), '-s', str(FS), '-OwS2', '-EFreverb=d', '-EFchorus=d',
                   '-o', wav + '.tmp.wav', mid]; env = dict(os.environ)
        else:
            render_fluid(font, mid, wav + '.tmp.wav'); cmd = None
        if cmd:
            r = subprocess.run(cmd, env=env, capture_output=True, text=True)
            log = r.stdout + r.stderr
            if r.returncode != 0 or not os.path.exists(wav + '.tmp.wav'):
                raise RuntimeError('render failed: %s\n%s' % (' '.join(cmd), log[-2000:]))
            open(wav + '.log', 'w').write(log)
        os.replace(wav + '.tmp.wav', wav)
    if os.path.exists(wav + '.log'): log = open(wav + '.log').read()
    x = sc.load_wav(wav)
    m = re.search(r'peak [\d.]+: the whole song (-?[\d.]+) dB down', log)
    if m: x = x * 10 ** (-float(m.group(1)) / 20)  # (undo pajupilli's mastering)
    return x, wav, log


# ---------------------------------------------------------------------------------------------
# The phrase probe

PH_LEG = [0, 2, 4, 5, 7, 9]  # (a major scale, six notes)
PH_STEP, PH_VEL = 0.4, 80


def phrase_layout(p):
    keys = sc.test_keys(p)
    base = keys[len(keys) // 2 - 2] if len(keys) > 4 else keys[0]
    ev = []
    for i, d in enumerate(PH_LEG):  # legato: each note held 20 ms into the next
        t = i * PH_STEP
        ev.append((t, 'on', base + d)); ev.append((t + PH_STEP + (0.02 if i < len(PH_LEG) - 1 else -0.05), 'off', base + d))
    t0 = len(PH_LEG) * PH_STEP + 1.0
    st = [t0 + i * PH_STEP for i in range(4)]
    for t in st:
        ev.append((t, 'on', base)); ev.append((t + 0.1, 'off', base))
    return base, [i * PH_STEP for i in range(len(PH_LEG))], st, ev, st[-1] + 2.5


def write_phrase(path, p):
    base, leg, st, ev, end = phrase_layout(p)
    tpq = 480; tick = lambda s: int(round(s * 2 * tpq))
    E = [(0, bytes([0xC0, p]))] + [(tick(t), bytes([0x90 if k == 'on' else 0x80, key, PH_VEL if k == 'on' else 0])) for t, k, key in ev]
    E.sort(key=lambda e: (e[0], e[1][0] == 0x90))
    body = bytearray(); last = 0
    for t, e in E: body += sc._enc_vlq(t - last) + e; last = t
    body += sc._enc_vlq(tick(end) - last) + b'\xff\x2f\x00'
    open(path, 'wb').write(b'MThd' + sc.struct.pack('>IHHH', 6, 0, 1, tpq) + b'MTrk' + sc.struct.pack('>I', len(body)) + bytes(body))


def phrase_measures(x, p):
    base, leg, st, ev, end = phrase_layout(p)
    m = x[:, 0] + x[:, 1]; pw = m ** 2; n1 = len(pw) // 48
    env = sc.db(pw[:n1 * 48].reshape(n1, 48).mean(axis=1))  # 1 ms
    M = {}
    on = []
    for t in leg + st:  # onsets: the steepest rise in -20..+150 ms
        a, b = max(int(t * 1000) - 20, 0), int(t * 1000) + 150
        e5 = np.convolve(env[a:b + 5], np.ones(5) / 5, 'valid')
        on.append(a + int(np.argmax(np.diff(e5))) - int(t * 1000))
    M['onset_ms'] = sc.r1(np.median(on), 0); M['onset_jitter_ms'] = sc.r1(np.std(on), 1)
    pk = [float(env[int(t * 1000):int(t * 1000) + 300].max()) for t in leg]
    M['legato_even_db'] = sc.r1(np.std(pk))
    dips = []
    for t in leg[1:]:
        i = int(t * 1000)
        before = float(np.mean(env[i - 120:i - 20])); low = float(env[i - 30:i + 60].min())
        dips.append(before - low)
    M['legato_dip_db'] = sc.r1(np.median(dips))
    ln = []
    for t in st:
        i = int(t * 1000); seg = env[i:i + int(PH_STEP * 1000)]
        pkk = seg.max(); j = int(np.argmax(seg)); below = np.nonzero(seg[j:] < pkk - 20)[0]
        ln.append(j + (below[0] if len(below) else len(seg) - j))
    M['staccato_len_ms'] = sc.r1(np.median(ln), 0)
    tail = env[int((st[-1] + 0.1) * 1000):]
    pk_last = float(env[int(st[-1] * 1000):int(st[-1] * 1000) + 100].max())
    below = np.nonzero(tail < pk_last - 40)[0]
    M['phrase_tail_ms'] = float(below[0]) if len(below) else float(len(tail))
    return M


# ---------------------------------------------------------------------------------------------
# One instrument


def pj_names(pjbin):
    r = subprocess.run([pjbin, '-inst'], capture_output=True, text=True); txt = r.stdout + r.stderr
    names = {}
    for n, nm in re.findall(r'(\d+) ([a-z0-9+\-]+)', txt.split('Not a General MIDI')[0]):
        names[int(n) - 1] = nm
    return names, txt


def profile_notes(x, layout, drum):
    nl = []
    for key, v, t0 in layout:
        n = sc.note_profile(x, t0, key, drum)
        if n is not None: n['vel'] = v; nl.append(n)
    return nl


def zimt_pair(xa, xb, la, lb, Z, vel=96):
    da = {(k, v): t for k, v, t in la}; db_ = {(k, v): t for k, v, t in lb}; d = []
    for kv in sorted(set(da) & set(db_)):
        if kv[1] != vel: continue
        sa = xa[int(da[kv] * FS):int((da[kv] + 3) * FS)]; sb = xb[int(db_[kv] * FS):int((db_[kv] + 3) * FS)]
        if len(sa) < FS or len(sb) < FS: continue
        ra, rb = np.sqrt((sa ** 2).mean()), np.sqrt((sb ** 2).mean())
        if ra <= 0 or rb <= 0: continue
        d.append(Z.distance((sa / ra * 0.1).astype(np.float32), (sb / rb * 0.1).astype(np.float32)))
    return float(np.median(d)) if d else None


def run_one(p, pjbin, workroot, pjname, zimt=True):
    try:
        return _run_one(p, pjbin, workroot, pjname, zimt)
    finally:
        shutil.rmtree(os.path.join(workroot, pkey(p)), ignore_errors=True)


def _run_one(p, pjbin, workroot, pjname, zimt=True):
    t_start = time.time()
    key = pkey(p); drum = p == 'drums'
    work = os.path.join(workroot, key); os.makedirs(work, exist_ok=True)
    keys = sc.test_keys(p)
    mid = os.path.join(work, key + '_notes.mid'); layout = sc.write_diag(mid, p, keys)
    pmid = None
    if not drum:
        pmid = os.path.join(work, key + '_phrase.mid'); write_phrase(pmid, p)
    profs = {}; audio = {}; logs = {}; phr = {}; times = {}
    for name, kind, font in SYNTHS:
        t = time.time()
        x, wav, log = render(name, kind, font, mid, work, pjbin)
        notes = profile_notes(x, layout, drum)
        profs[name] = dict(notes=notes, summary=sc.instrument_summary(notes, drum))
        audio[name] = x.mean(axis=1).astype(np.float32)  # (for Zimtohrli; the render itself is deleted now)
        for f in (wav, wav + '.log'):
            if os.path.exists(f): os.remove(f)
        if kind == 'pj': logs[name] = log
        if pmid:
            xp, pwav, plog = render(name, kind, font, pmid, work, pjbin)
            for f in (pwav, pwav + '.log'):
                if os.path.exists(f): os.remove(f)
            try: phr[name] = phrase_measures(xp, p)
            except Exception as e: phr[name] = dict(error=str(e))
        times[name] = round(time.time() - t, 1)
    # pairwise note-for-note distances (pajupilli and every reference; check pairs)
    pair = {}
    names = ['pajupilli'] + REFS
    for m, (label, f, wt) in sc.PAIR_MEASURES.items():
        D = {}
        for i in range(len(names)):
            for j in range(i + 1, len(names)):
                v = sc._pair_median(profs[names[i]], profs[names[j]], f)
                if v is not None: D['%s|%s' % (names[i], names[j])] = round(v, 4)
        for a, b in CHECK:
            v = sc._pair_median(profs[a], profs[b], f)
            if v is not None: D['%s|%s' % (a, b)] = round(v, 4)
        pair[m] = D
    Z = sc.zimt() if zimt else None
    if Z is not None:
        D = {}
        for i in range(len(names)):
            for j in range(i + 1, len(names)):
                v = zimt_pair(audio[names[i]], audio[names[j]], layout, layout, Z)
                if v is not None: D['%s|%s' % (names[i], names[j])] = round(v, 5)
        pair['zimtohrli'] = D
    silent = {n: sum(1 for q in profs[n]['notes'] if q.get('silent')) for n in profs}
    res = dict(key=key, program=p, gm='Drum kit (channel 10)' if drum else GM[p], pajupilli=pjname, keys=keys,
               vels=list(sc.VELS), made=time.strftime('%Y-%m-%d %H:%M'), seconds=times,
               summaries={n: profs[n]['summary'] for n in profs}, silent=silent, phrase=phr, pair=pair,
               pj_log=[l for l in logs.get('pajupilli', '').splitlines() if 'channel' in l or 'velocit' in l or 'peak' in l][:6])
    if drum:  # per drum key: level and centroid at velocity 96, for the per-piece table
        per = {}
        for n in profs:
            d = {}
            for q in profs[n]['notes']:
                if q.get('vel') == 96 and 'centroid_erb' in q:
                    d[str(q['key'])] = [q['level_db'], q['centroid_erb'], q.get('decay_db_s'), q.get('attack_ms')]
            per[n] = d
        res['drum_keys'] = per
    os.makedirs(DATA, exist_ok=True)
    json.dump(res, open(os.path.join(DATA, key + '.json.tmp'), 'w'), indent=0, default=lambda o: o.tolist() if isinstance(o, np.ndarray) else str(o))
    os.replace(os.path.join(DATA, key + '.json.tmp'), os.path.join(DATA, key + '.json'))
    shutil.rmtree(work, ignore_errors=True)  # (the renders: tens of MB each)
    return key, round(time.time() - t_start)


def parse_programs(s, pjnames):
    out = []
    for tok in s.split(','):
        tok = tok.strip()
        if not tok: continue
        if tok == 'drums': out.append('drums')
        elif re.fullmatch(r'\d+-\d+', tok):
            a, b = map(int, tok.split('-')); out += list(range(a, b + 1))
        elif tok.isdigit(): out.append(int(tok))
        else:
            hit = [k for k, v in pjnames.items() if v == tok]
            if not hit: raise SystemExit('unknown instrument ' + tok)
            out += hit
    return out


def cmd_run(a):
    pjbin = os.path.abspath(a.bin)
    names, txt = pj_names(pjbin)
    progs = parse_programs(a.programs, names)
    if not a.force:
        progs = [p for p in progs if not os.path.exists(os.path.join(DATA, pkey(p) + '.json'))]
    os.makedirs(a.work, exist_ok=True); os.makedirs(DATA, exist_ok=True)
    meta = os.path.join(DATA, '_build.json')
    commit = subprocess.run(['git', '-C', ROOT, 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
    stat = subprocess.run(['git', '-C', ROOT, 'diff', '--stat'], capture_output=True, text=True).stdout
    md5 = hashlib.md5(open(pjbin, 'rb').read()).hexdigest()
    M = json.load(open(meta)) if os.path.exists(meta) else {}
    M.setdefault('builds', {})[md5] = dict(bin=pjbin, commit=commit, diff_stat=stat, first_used=time.strftime('%Y-%m-%d %H:%M'))
    tv = subprocess.run(['timidity', '--version'], capture_output=True, text=True).stdout.splitlines()[:1]
    M['tools'] = dict(timidity=tv, fluidsynth='libfluidsynth.so.3',
                      fonts={k: dict(path=v, bytes=os.path.getsize(v)) for k, v in SF.items()})
    json.dump(M, open(meta, 'w'), indent=1)
    print('%d instruments to do, %d jobs' % (len(progs), a.jobs), file=sys.stderr)
    with ProcessPoolExecutor(max_workers=a.jobs) as ex:
        fut = {ex.submit(run_one, p, pjbin, a.work, names.get(p, 'drum kit') if p != 'drums' else 'drum kit', not a.no_zimt): p for p in progs}
        for f in as_completed(fut):
            p = fut[f]
            try:
                k, s = f.result()
                d = json.load(open(os.path.join(DATA, k + '.json'))); d['build_md5'] = md5
                json.dump(d, open(os.path.join(DATA, k + '.json'), 'w'), indent=0)
                print('done %s (%s) in %d s' % (k, names.get(p, p), s), file=sys.stderr, flush=True)
            except Exception:
                print('FAILED %s\n%s' % (p, traceback.format_exc()), file=sys.stderr, flush=True)


# ---------------------------------------------------------------------------------------------
# Scoring and the report

PHRASE_MEASURES = {  # (label, unit, JND, weight, up, down)
    'onset_ms': ('onset in a phrase (re the note-on)', 'ms', 10, 0.6, 'speaks later in a phrase', 'speaks sooner in a phrase'),
    'onset_jitter_ms': ('onset jitter in a phrase', 'ms', 5, 0.3, 'more uneven onsets', 'more even onsets'),
    'legato_even_db': ('level evenness of a legato scale', 'dB', 1, 0.5, 'a less even scale', 'a more even scale'),
    'legato_dip_db': ('dip between legato notes', 'dB', 2, 0.5, 'more separated legato', 'smoother legato'),
    'staccato_len_ms': ('sounding length of a 100 ms note', 'ms', None, 0.6, 'longer staccato', 'shorter staccato'),
}
PH_LOG = {'staccato_len_ms': 0.25}
ALL_MEASURES = dict(sc.SUMMARY_MEASURES); ALL_MEASURES.update(PHRASE_MEASURES)
LOG = dict(sc.LOGRATIO); LOG.update(PH_LOG)
# Measures that pajupilli's room (every instrument plays in its own room; the fonts are dry) moves for
# nearly every instrument: listed per instrument, left out of the distance and the summary table.
ROOM = {'floor_db', 'release_ms', 'icc', 'staccato_len_ms'}
FLAGS = os.path.join(ROOT, 'research', 'gm_compare', 'flags.json')  # {key: few words}: shown in the table


def score_measure(m, a, refs):
    label, unit, jnd, wt, up, down = ALL_MEASURES[m]
    if a is None or len([r for r in refs if r is not None]) < 2: return None
    if m in LOG:
        if a <= 0 or any(r is not None and r <= 0 for r in refs): return None
        tr = math.log2; jnd = LOG[m]
    else:
        tr = lambda v: v
    ta = tr(a); rv = [tr(r) for r in refs if r is not None]
    med = float(np.median(rv)); spread = (max(rv) - min(rv)) / 2
    d = ta - med; z = abs(d) / max(jnd, spread)
    outside = ta < min(rv) - jnd / 2 or ta > max(rv) + jnd / 2
    beyond = max(0.0, min(rv) - ta, ta - max(rv)) / jnd
    return dict(measure=m, label=label, unit=unit, a=a, refs=refs, d=d, z=z, spread_jnd=spread / jnd, outside=outside, beyond=beyond,
                word=up if d > 0 else down, score=min(z, 8) * wt * (1 if outside else 0.5), log=m in LOG)


def score_pair(m, P):
    label, wt = (sc.PAIR_MEASURES[m][0], sc.PAIR_MEASURES[m][2]) if m in sc.PAIR_MEASURES else \
        ('Zimtohrli per note (velocity 96, loudness matched)', 0.4)
    dar = [P.get('pajupilli|' + r) for r in REFS]; dar = [x for x in dar if x is not None]
    drr = [P.get('%s|%s' % (REFS[i], REFS[j])) for i in range(len(REFS)) for j in range(i + 1, len(REFS))]
    drr = [x for x in drr if x is not None]
    if not dar or not drr: return None
    yard = float(np.median(drr)); dm = float(np.median(dar))
    z = dm / yard if yard > 0 else 1.0
    nearest = min(dar)
    outside = nearest > 1.3 * yard  # (even the nearest font is further than the fonts are from each other)
    return dict(measure=m, label=label, a=dm, nearest=nearest, yard=yard, z=z, outside=outside, spread_jnd=None,
                word='further from the fonts than they are from each other' if z > 1 else 'within the fonts\' own spread',
                score=min(z, 8) * wt * 0.6 * (1 if outside else 0.5))


def fmt(v, unit=''):
    if v is None: return '-'
    if isinstance(v, (int, float)):
        s = ('%.3g' % v) if abs(v) < 10 else ('%.1f' % v) if abs(v) < 100 else '%.0f' % v
        return s + (' ' + unit if unit and unit != '' else '')
    return str(v)


def load_all():
    R = {}
    if not os.path.isdir(DATA): return R
    for f in sorted(os.listdir(DATA)):
        if f.endswith('.json') and not f.startswith('_'):
            d = json.load(open(os.path.join(DATA, f))); R[d['key']] = d
    return R


def analyse(R):
    """Scores every instrument; the level against the other instruments needs all of them."""
    names = ['pajupilli'] + REFS
    base = {n: float(np.median([d['summaries'][n].get('level96_db', -99) for d in R.values() if d['key'] != 'drums']))
            for n in names} if R else {}
    out = {}
    for k, d in R.items():
        S = d['summaries']
        for n in names: S[n]['level_rel_db'] = sc.r1(S[n].get('level96_db', -99) - base[n])
        its = []
        for m in sc.SUMMARY_MEASURES:
            if m not in S['pajupilli']: continue
            r = score_measure(m, S['pajupilli'][m], [S[n].get(m) for n in REFS])
            if r: its.append(r)
        if d.get('phrase') and 'pajupilli' in d['phrase']:
            for m in PHRASE_MEASURES:
                r = score_measure(m, d['phrase']['pajupilli'].get(m), [d['phrase'].get(n, {}).get(m) for n in REFS])
                if r: its.append(r)
        for m, P in d['pair'].items():
            r = score_pair(m, P)
            if r: its.append(r)
        its.sort(key=lambda i: -i['score'])
        top = [i['score'] for i in its if i['measure'] not in ROOM][:8]
        dist = float(np.mean(top)) if top else 0.0
        sp = [i['spread_jnd'] for i in its if i.get('spread_jnd') is not None]
        yards = [i['yard'] / i['a'] for i in its if 'yard' in i and i['a'] > 0]
        disagree = float(np.median(sp)) if sp else 0.0
        # TiMidity against FluidSynth for the same font (the check)
        chk = []
        for a_, b_ in CHECK:
            if a_ in S and b_ in S and 'centroid96_erb' in S[a_] and 'centroid96_erb' in S[b_]:
                chk.append((a_, S[b_]['centroid96_erb'] - S[a_]['centroid96_erb'],
                            (S[b_].get('attack_bright_erb') or 0) - (S[a_].get('attack_bright_erb') or 0),
                            d['pair'].get('envelope', {}).get('%s|%s' % (a_, b_)),
                            (S[b_].get('bright_vel_erb') or 0) - (S[a_].get('bright_vel_erb') or 0),
                            (S[b_].get('vel_range_db') or 0) - (S[a_].get('vel_range_db') or 0)))
        out[k] = dict(items=its, distance=dist, disagree=disagree, check=chk)
    return out


def region_diff(S, n):
    """Mean spectrum of pajupilli re font n by region (level removed)."""
    a, b = S['pajupilli'].get('spectrum_db'), S[n].get('spectrum_db')
    if not a or not b: return None
    a, b = np.array(a), np.array(b); FC = sc.FC[:len(a)]
    ok = (a > -50) | (b > -50); d = a - b
    if not ok.any(): return None
    dd = np.where(ok, d - d[ok].mean(), 0); r = []
    for lo, hi in ((40, 150), (150, 400), (400, 1000), (1000, 2500), (2500, 6000), (6000, 16000)):
        sel = ok & (FC >= lo) & (FC < hi)
        r.append(round(float(dd[sel].mean()), 1) if sel.any() else None)
    return r


REGIONS = ['<150 Hz', '150-400', '400-1k', '1-2.5k', '2.5-6k', '>6k']


def short_phrase(it):
    """A few words for the summary table."""
    m = it['measure']
    if 'yard' in it:
        return {'spectrum': 'body spectrum shape', 'attack_spectrum': 'attack spectrum', 'envelope': 'envelope shape',
                'sgram': 'spectrogram over time', 'overtones': 'overtone balance', 'zimtohrli': 'Zimtohrli distance'}[m] + \
            ' (%.1fx fonts\' spread)' % it['z']
    a = it['a']; med = float(np.median([r for r in it['refs'] if r is not None]))
    if it['log']:
        return '%s (%s vs %s %s)' % (it['word'], fmt(a), fmt(med), it['unit'])
    return '%s (%s vs %s%s)' % (it['word'], fmt(a), fmt(med), ' ' + it['unit'] if it['unit'] else '')


def parse_notes():
    if not os.path.exists(NOTES): return {}, ''
    txt = open(NOTES).read(); N = {}; head = []
    cur = None
    for line in txt.splitlines():
        mm = re.match(r'^## (\S+)\s*$', line)
        if mm: cur = mm.group(1); N[cur] = []; continue
        if cur: N[cur].append(line)
        else: head.append(line)
    return {k: '\n'.join(v).strip() for k, v in N.items()}, '\n'.join(head)


def cmd_report(a):
    R = load_all()
    if not R: raise SystemExit('no data in ' + DATA)
    A = analyse(R); notes, _ = parse_notes()
    flags = json.load(open(FLAGS)) if os.path.exists(FLAGS) else {}
    pjn, _ = pj_names(os.path.abspath(a.bin))
    for k, d in R.items():
        if k != 'drums' and d['program'] in pjn: d['pajupilli'] = pjn[d['program']]
    order = sorted(R, key=lambda k: (k == 'drums', R[k]['program'] if k != 'drums' else 0))
    rank = sorted(order, key=lambda k: -A[k]['distance'])
    top15 = set(rank[:15])
    meta = json.load(open(os.path.join(DATA, '_build.json'))) if os.path.exists(os.path.join(DATA, '_build.json')) else {}
    L = []
    hand = open(a.head).read().rstrip() + '\n' if a.head and os.path.exists(a.head) else ''
    L.append(hand)
    L += ['## Summary table', '',
          'One row per instrument. **Distance**: the mean of the eight largest weighted differences (each in units of the larger '
          'of its JND and half the fonts\' own range, capped at 8, halved when pajupilli lies inside the fonts\' range), leaving out '
          'the four measures pajupilli\'s room moves (release, staccato length, floor, stereo coherence); about 1 '
          'is "like another SoundFont", 3 and more stands out. **Fonts disagree**: the median, over the measures, of half the four '
          'references\' range in JNDs (high: the fonts themselves are far apart, so a difference there is less likely a fault). '
          '**Rank** marks the 15 furthest (the first to look at). The differences are pajupilli against the fonts\' median.', '',
          '| GM | instrument | pajupilli | distance | rank | fonts disagree | biggest differences |', '|---|---|---|---|---|---|---|']
    for k in order:
        d = R[k]; s = A[k]
        dis = s['disagree']; lab = 'high' if dis >= 3 else 'medium' if dis >= 1.5 else 'low'
        big = '; '.join(short_phrase(i) for i in [x for x in s['items'] if x['measure'] not in ROOM][:3])
        if flags.get(k): big = '**%s**; %s' % (flags[k], big)
        rk = rank.index(k) + 1
        L.append('| %s | %s | %s | %.2f | %s | %s (%.1f) | %s |' % ('-' if k == 'drums' else d['program'] + 1, d['gm'], d['pajupilli'],
                                                                 s['distance'], '**%d**' % rk if k in top15 else str(rk), lab, dis, big))
    L += ['', '### The 15 furthest', '']
    for i, k in enumerate(rank[:15]):
        d = R[k]; s = A[k]
        L.append('%d. **%s %s** (%s, distance %.2f): %s.' % (i + 1, '' if k == 'drums' else d['program'] + 1, d['gm'], d['pajupilli'],
                                                              s['distance'], ('**%s**; ' % flags[k] if flags.get(k) else '') + '; '.join(short_phrase(x) for x in [y for y in s['items'] if y['measure'] not in ROOM][:3])))
    if a.patterns and os.path.exists(a.patterns): L += ['', open(a.patterns).read().rstrip()]
    L += ['', '## Per instrument', '']
    for k in order:
        d = R[k]; s = A[k]; S = d['summaries']
        L += ['### %s %s: %s' % ('' if k == 'drums' else d['program'] + 1, d['gm'], d['pajupilli']), '']
        L.append('Distance %.2f (rank %d of %d); the fonts disagree %.1f JND. Keys %s, velocities %s.' % (
            s['distance'], rank.index(k) + 1, len(rank), s['disagree'], ','.join(map(str, d['keys'])) if k != 'drums' else '35-81',
            '/'.join(map(str, d['vels']))))
        if notes.get(k): L += ['', notes[k]]
        L += ['', 'Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):', '']
        n = 0
        for it in s['items']:
            if n >= 6 or it['score'] < 0.3: break
            n += 1
            if it['measure'] in ROOM: it = dict(it, label=it['label'] + ' [room]')
            if 'yard' in it:
                L.append('%d. **%s**: pajupilli %s (median distance to the fonts %.3g, nearest %.3g; the fonts among themselves %.3g)%s.' % (
                    n, it['label'], it['word'], it['a'], it['nearest'], it['yard'], ', outside' if it['outside'] else ''))
            else:
                L.append('%d. **%s**: %s (%s | %s)%s, %.1f x.' % (n, it['label'], it['word'], fmt(it['a'], it['unit']),
                                                               ' | '.join(fmt(r) for r in it['refs']), ', outside the fonts' if it['outside'] else ', inside the fonts\' range', it['z']))
        if n == 0: L.append('(nothing above the threshold)')
        rows = []
        for nn in REFS:
            r = region_diff(S, nn)
            if r: rows.append('| re %s | %s |' % (nn, ' | '.join('%+.0f' % v if v is not None else '-' for v in r)))
        if rows:
            L += ['', 'Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):', '',
                  '| | ' + ' | '.join(REGIONS) + ' |', '|---|' + '---|' * len(REGIONS)] + rows
        vc = ['%s %s' % (nn, ' '.join('%+.0f' % S[nn].get('vel_curve_db', {}).get(str(v), float('nan')) for v in d['vels'])) for nn in ['pajupilli'] + REFS]
        L += ['', '- velocity curve, dB re velocity 96 at %s: %s' % ('/'.join(map(str, d['vels'])), '; '.join(vc))]
        chk = [c for c in s['check'] if abs(c[1]) >= 0.5 or abs(c[2]) >= 0.5 or (c[3] or 0) >= 4 or abs(c[4]) >= 0.5 or abs(c[5]) >= 3]
        if chk:
            L.append('- TiMidity misreads (FluidSynth re TiMidity, same font): ' + '; '.join(
                '%s: centroid %+.1f ERB, attack %+.1f ERB, velocity brightening %+.1f ERB, loudness range %+.0f dB, envelope %.1f dB rms'
                % (c[0], c[1], c[2], c[4], c[5], c[3] or 0) for c in chk))
        sil = {n: v for n, v in d.get('silent', {}).items() if v}
        if sil: L.append('- silent probe notes: ' + ', '.join('%s %d' % kv for kv in sil.items()))
        if k == 'drums' and 'drum_keys' in d:
            L += ['', '<details><summary>per drum key: level (dB re the kit median) and centroid (ERB), velocity 96</summary>', '',
                  '| key | ' + ' | '.join(['pajupilli'] + REFS) + ' |', '|---|' + '---|' * 5]
            med = {n: np.median([v[0] for v in d['drum_keys'].get(n, {}).values()] or [0]) for n in ['pajupilli'] + REFS}
            for key in range(35, 82):
                cells = []
                for n in ['pajupilli'] + REFS:
                    v = d['drum_keys'].get(n, {}).get(str(key))
                    cells.append('%+.0f / %.1f' % (v[0] - med[n], v[1]) if v else '-')
                L.append('| %d | %s |' % (key, ' | '.join(cells)))
            L += ['', '</details>']
        L += ['', '<details><summary>every measure</summary>', '', '| measure | pajupilli | ' + ' | '.join(REFS) + ' | x | outside |',
              '|---|' + '---|' * 7]
        for it in sorted(s['items'], key=lambda i: i['measure']):
            if 'yard' in it:
                L.append('| %s | %.3g (nearest %.3g) | fonts among themselves %.3g | | | | %.1f | %s |' % (it['label'], it['a'], it['nearest'], it['yard'], it['z'], 'yes' if it['outside'] else ''))
            else:
                L.append('| %s | %s | %s | %.1f | %s |' % (it['label'] + (' (%s)' % it['unit'] if it['unit'] else ''), fmt(it['a']),
                                                           ' | '.join(fmt(r) for r in it['refs']), it['z'], 'yes' if it['outside'] else ''))
        L += ['', '</details>', '']
    if a.tail and os.path.exists(a.tail): L += [open(a.tail).read().rstrip(), '']
    open(REPORT, 'w').write('\n'.join(L) + '\n')
    json.dump({k: dict(distance=A[k]['distance'], disagree=A[k]['disagree'], rank=rank.index(k) + 1,
                       top=[dict(measure=i['measure'], text=short_phrase(i), score=round(i['score'], 2), outside=i['outside']) for i in A[k]['items'][:6]])
               for k in order}, open(os.path.join(os.path.dirname(DATA), 'scores.json'), 'w'), indent=1)
    print('wrote %s (%d instruments)' % (REPORT, len(R)), file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('cmd', choices=['run', 'report'])
    ap.add_argument('--bin', default=os.path.join(ROOT, 'pajupilli'))
    ap.add_argument('--programs', default='0-127,drums', help='GM programs 0-127 (0-based), ranges, pajupilli names, "drums"')
    ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--force', action='store_true', help='redo instruments that already have results')
    ap.add_argument('--work', default=os.path.expanduser('~/.cache/gmcompare'), help='renders (deleted per instrument)')
    ap.add_argument('--no-zimt', action='store_true')
    ap.add_argument('--head', default=os.path.join(ROOT, 'research', 'gm_compare', 'head.md'))
    ap.add_argument('--patterns', default=os.path.join(ROOT, 'research', 'gm_compare', 'patterns.md'))
    ap.add_argument('--tail', default=os.path.join(ROOT, 'research', 'gm_compare', 'methods.md'))
    a = ap.parse_args()
    cmd_run(a) if a.cmd == 'run' else cmd_report(a)


if __name__ == '__main__':
    main()
