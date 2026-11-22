#!/usr/bin/env python3
"""pajupilli against several SoundFonts, one General MIDI instrument at a time
(research/gm_compare.md).

    tools/gmcompare.py run --bin ./pajupilli --programs 0-127,drums [--jobs 8]
    tools/gmcompare.py run --bin /path/to/pajupilli --programs 40 --force   # re-run one
    tools/gmcompare.py report                                             # rebuild the report
    tools/gmcompare.py kits [--pj-only] [--measure 0|1|2]                  # the drum kits, stroke by stroke: research/gm_kits.md
    tools/gmcompare.py kits --emit                                        # ... and src/drumcal_data.h: each drum at the fonts' level
    tools/gmcompare.py levels [--pj-only] [--measure 0|1|2]                # the melodic programs against the piano: research/gm_levels.md
    tools/gmcompare.py levels --report-only --emit                         # ... and src/gmlevels_data.h: each program at the fonts' level
    tools/gmcompare.py keys [--pj-only] [--report-only --emit]             # each key against the instrument's others: research/gm_keys.md, src/gmkeys_data.h

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


# ---------------------------------------------------------------------------------------------
# The drum kits, stroke by stroke: how loud each piece is against the band

KITS = [('standard', 0), ('jazz', 32), ('brush', 40)]  # the drum channel's program
KIT_SLOT, KIT_HOLD = 4.0, 1.0
KIT_ANCHOR = (48, 60, 72)  # a piano's notes (channel 1, velocity 96), each synth's own yardstick
KIT_DATA = os.path.join(os.path.dirname(DATA), 'kits.json')
KIT_REPORT = os.path.join(ROOT, 'research', 'gm_kits.md')
DRUM_NAMES = ('Acoustic Bass Drum,Bass Drum 1,Side Stick,Acoustic Snare,Hand Clap,Electric Snare,Low Floor Tom,Closed Hi-Hat,'
              'High Floor Tom,Pedal Hi-Hat,Low Tom,Open Hi-Hat,Low-Mid Tom,Hi-Mid Tom,Crash Cymbal 1,High Tom,Ride Cymbal 1,'
              'Chinese Cymbal,Ride Bell,Tambourine,Splash Cymbal,Cowbell,Crash Cymbal 2,Vibraslap,Ride Cymbal 2,Hi Bongo,'
              'Low Bongo,Mute Hi Conga,Open Hi Conga,Low Conga,High Timbale,Low Timbale,High Agogo,Low Agogo,Cabasa,Maracas,'
              'Short Whistle,Long Whistle,Short Guiro,Long Guiro,Claves,Hi Wood Block,Low Wood Block,Mute Cuica,Open Cuica,'
              'Mute Triangle,Open Triangle').split(',')
DRUM_GROUPS = [('kick', (35, 36)), ('snare', (37, 38, 39, 40)), ('toms', (41, 43, 45, 47, 48, 50)), ('hi-hat', (42, 44, 46)),
               ('ride', (51, 53, 59)), ('crash', (49, 52, 55, 57)), ('other', tuple(k for k in range(54, 82) if k not in (55, 57, 59)))]


KIT_GRID = (4, 8, 16, 32, 48, 64, 80, 96, 112, 127)  # pajupilli's own curve, for --emit
KIT_BRUSHED = (41, 43, 45, 47, 48, 50, 42, 46, 49, 51, 52, 53, 55, 57, 59)  # the Brush kit's toms, hi-hat and cymbals
KIT_BRUSH_DB = -8.0  # ... are this far under the Standard kit's (the fonts' Brush kits play the stick's samples there)
KIT_HEADER = os.path.join(ROOT, 'src', 'drumcal_data.h')


def kit_layout(keys, vels=None):
    """(channel, key, velocity, t): the piano's yardstick notes, then every drum key, soft to loud."""
    L = [(0, k, 96) for k in KIT_ANCHOR] + [(1, 36, 0)] + [(9, k, v) for k in keys for v in (vels or sc.VELS)]
    return [(ch, k, v, i * KIT_SLOT) for i, (ch, k, v) in enumerate(L)]


def write_kit(path, program, keys, vels=None):
    tpq = 480; tick = lambda s: int(round(s * 2 * tpq))
    lay = kit_layout(keys, vels)
    ev = [(0, bytes([0xC0, 0])), (0, bytes([0xC9, program]))]
    # (pajupilli's Jazz kit is the kit of a file with a piano and an upright bass: one bass note in a slot of its own)
    if program == 32: lay[len(KIT_ANCHOR)] = (1, 36, 64, lay[len(KIT_ANCHOR)][3]); ev.append((0, bytes([0xC1, 32])))
    lay = [l for l in lay if l[2] > 0]
    for ch, k, v, t in lay:
        ev.append((tick(t), bytes([0x90 | ch, k, v]))); ev.append((tick(t + KIT_HOLD), bytes([0x80 | ch, k, 0])))
    ev.sort(key=lambda e: (e[0], e[1][0] & 0xF0 == 0x90))
    body = bytearray(); last = 0
    for t, e in ev: body += sc._enc_vlq(t - last) + e; last = t
    body += sc._enc_vlq(tick(KIT_SLOT)) + b'\xff\x2f\x00'
    open(path, 'wb').write(b'MThd' + sc.struct.pack('>IHHH', 6, 0, 1, tpq) + b'MTrk' + sc.struct.pack('>I', len(body)) + bytes(body))
    return lay


def kit_cfg(font, work):
    """TiMidity's cfg with the three drum sets of this font mapped (sc.timidity_cfg maps drum set 0 only)."""
    sf = SF[font]; cfg = os.path.join(work, 'kit_%s.cfg' % font)
    L = ['soundfont "%s"' % sf, 'bank 0'] + ['%d %%font "%s" 0 %d' % (p, sf, p) for p in range(128)]
    for _, prog in KITS:
        L += ['drumset %d' % prog] + ['%d %%font "%s" 128 %d %d' % (k, sf, prog, k) for k in range(27, 88)]
    open(cfg, 'w').write('\n'.join(L) + '\n')
    return cfg


def stroke_levels(x, t):
    """One stroke's loudness as a meter reads it (BS.1770's weighting): its loudest 400 ms, and all of it
    (its energy over the slot, as if spread over one second); and the plain peak."""
    seg = x[int(t * FS):int((t + KIT_SLOT - 0.05) * FS)]
    if len(seg) < FS: return None
    p = (sc.kweight(seg) ** 2).sum(axis=1)
    B, H = int(0.4 * FS), int(0.02 * FS)
    c = np.concatenate([[0], np.cumsum(p)]); idx = np.arange(0, len(p) - B, H)
    mom = float(sc.db(((c[idx + B] - c[idx]) / B).max()))
    return [sc.r1(mom), sc.r1(sc.db(p.sum() / FS)), sc.r1(20 * math.log10(max(float(np.abs(seg).max()), 1e-7)))]


def kit_one(kit, program, name, kind, font, pjbin, workroot, raw=False):
    work = os.path.join(workroot, 'kit_%s_%s' % (kit, name.replace('/', '_'))); os.makedirs(work, exist_ok=True)
    try:
        mid = os.path.join(work, kit + '.mid'); lay = write_kit(mid, program, list(range(35, 82)), KIT_GRID if raw else None)
        wav = os.path.join(work, 'out.wav'); log = ''
        if kind == 'pj':
            r = subprocess.run([pjbin, '-director', 'off', '-play', mid, '-o', wav],
                               env=dict(os.environ, PJ_PLAY_VELMEDIAN='1000', PJ_THREADS='3', PJ_SWIRL='0',
                                        **(dict(PJ_NODRUMCAL='1') if raw else {})), capture_output=True, text=True)
            log = r.stdout + r.stderr
            if r.returncode != 0: raise RuntimeError(log[-2000:])
        elif kind == 'tim':
            r = subprocess.run(['timidity', '-c', kit_cfg(font, work), '-s', str(FS), '-OwS2', '-EFreverb=d', '-EFchorus=d',
                                '-o', wav, mid], capture_output=True, text=True)
            if r.returncode != 0 or not os.path.exists(wav): raise RuntimeError((r.stdout + r.stderr)[-2000:])
        else:
            render_fluid(font, mid, wav)
        x = sc.load_wav(wav)
        m = re.search(r'peak [\d.]+: the whole song (-?[\d.]+) dB down', log)
        if m: x = x * 10 ** (-float(m.group(1)) / 20)  # (undo pajupilli's mastering)
        anchor = []; S = {}
        for ch, k, v, t in lay:
            l = stroke_levels(x, t)
            if l is None: continue
            if ch == 0: anchor.append(l)
            elif ch == 9: S['%d/%d' % (k, v)] = l
        return kit, name, dict(anchor=[sc.r1(np.mean([a[i] for a in anchor])) for i in range(3)], strokes=S,
                               log=[l for l in log.splitlines() if 'channel' in l or 'peak' in l or 'drum' in l][:12])
    finally:
        shutil.rmtree(work, ignore_errors=True)


def cmd_kits(a):
    if not a.report_only:
        synths = [s for s in SYNTHS if s[0] == 'pajupilli' or s[0] in REFS]
        R = json.load(open(KIT_DATA)) if os.path.exists(KIT_DATA) and (a.pj_only or a.emit or a.refine) else {}
        if a.refine: return kits_emit(R, True)
        if a.pj_only: synths = synths[:1]
        if a.emit:  # pajupilli's own curves, uncalibrated; the fonts only if not at hand
            synths = [('pajupilli/raw', 'pj', None)] + ([] if all(n in R.get(k, {}) for k, _ in KITS for n in REFS) else synths[1:])
        with ProcessPoolExecutor(a.jobs) as ex:
            fut = [ex.submit(kit_one, kit, prog, n, kind, font, a.bin, a.work, n == 'pajupilli/raw')
                   for kit, prog in KITS for n, kind, font in synths]
            for f in as_completed(fut):
                kit, name, res = f.result(); R.setdefault(kit, {})[name] = res
                if name == 'pajupilli/raw': R.pop('adjust', None)
                print('%s %s' % (kit, name), file=sys.stderr)
        json.dump(R, open(KIT_DATA, 'w'), indent=0)
    if a.emit: return kits_emit(R)
    R = json.load(open(KIT_DATA)); R.pop('adjust', None)
    kits_report(R, a.measure)


def kits_rel(R, measure=0):
    """kit -> synth -> 'key/velocity' -> the stroke against that synth's own piano (dB); silent strokes left out."""
    return {kit: {n: {s: l[measure] - D[n]['anchor'][measure] for s, l in D[n]['strokes'].items() if l[2] > -80} for n in D}
            for kit, D in R.items()}


def kits_target(rel, kit, k, v):
    """What a stroke should be against the piano: the SoundFonts' average; the Brush kit's toms, hi-hat and
    cymbals KIT_BRUSH_DB under the Standard kit's. ((target, the fonts' range) or None.)"""
    if kit == 'brush' and k in KIT_BRUSHED:
        t = kits_target(rel, 'standard', k, v)
        return (t[0] + KIT_BRUSH_DB, t[1]) if t else None
    s = '%d/%d' % (k, v); refs = [rel[kit][n][s] for n in REFS if n in rel[kit] and s in rel[kit][n]]
    return (float(np.mean(refs)), max(refs) - min(refs)) if len(refs) >= 2 else None


def kits_table(R, measure=0):
    """kit -> key -> velocity -> (pajupilli over the target, the SoundFonts' range, pajupilli, the target), each
    synth's stroke taken against its own piano; nothing where pajupilli or the fonts are silent."""
    T = {}; rel = kits_rel(R, measure)
    for kit in R:
        T[kit] = {}
        for k in range(35, 82):
            row = {}
            for v in sc.VELS:
                s = '%d/%d' % (k, v); t = kits_target(rel, kit, k, v)
                if s in rel[kit].get('pajupilli', {}) and t:
                    row[v] = (rel[kit]['pajupilli'][s] - t[0], t[1], rel[kit]['pajupilli'][s], t[0])
            T[kit][k] = row
    return T


def kits_emit(R, refine=False):
    """src/drumcal_data.h: per kit and key, the target at sc.VELS and pajupilli's own curve at KIT_GRID.
    refine: what the calibrated pajupilli (the last `kits` run) still misses is taken off the target it is
    given (R['adjust'], kept and added to; at most 10 dB): for what the stroke alone does not reach as the
    curve says (a drum's hands and places differ with the velocity)."""
    rel = kits_rel({k: v for k, v in R.items() if k != 'adjust'}); L = []
    adj = R.setdefault('adjust', {})
    if refine:
        T = kits_table({k: v for k, v in R.items() if k != 'adjust'})
        for kit in T:
            for k, row in T[kit].items():
                a = adj.setdefault(kit, {}).setdefault(str(k), [0.0] * len(sc.VELS))
                for i, v in enumerate(sc.VELS):
                    if v in row: a[i] = round(float(np.clip(a[i] + 0.8 * row[v][0], -10, 10)), 2)
        json.dump(R, open(KIT_DATA, 'w'), indent=0)
    for i, (kit, _) in enumerate(KITS):
        own = rel[kit].get('pajupilli/raw', {})
        for k in range(35, 82):
            t = [kits_target(rel, kit, k, v) for v in sc.VELS]; o = [own.get('%d/%d' % (k, u)) for u in KIT_GRID]
            if None in t or o[-1] is None: continue
            for j in range(len(o) - 2, -1, -1):  # (too soft to measure: as the loudness curve would have it)
                if o[j] is None: o[j] = o[j + 1] + 33.3 * math.log10(KIT_GRID[j] / KIT_GRID[j + 1])
            a = adj.get(kit, {}).get(str(k), [0.0] * len(sc.VELS)); t = [(x[0] - a[j], x[1]) for j, x in enumerate(t)]
            L.append('    {%d, %d, {%s}, {%s}},  // %s' % (i, k, ', '.join('%.1ff' % x[0] for x in t), ', '.join('%.1ff' % x for x in o),
                                                          DRUM_NAMES[k - 35]))
    H = ['// Generated by tools/gmcompare.py kits --emit: do not edit.',
         '// Per kit (0 Standard, 1 Jazz, 2 Brush) and General MIDI drum key: how loud the stroke should be at',
         '// velocities kDrumCalVels (the SoundFonts\' average; the Brush kit\'s toms, hi-hat and cymbals %g dB under the' % -KIT_BRUSH_DB,
         '// Standard kit\'s) and how loud pajupilli\'s own is at kDrumCalGrid, uncalibrated: each its loudest 400 ms,',
         '// K-weighted, against a piano\'s notes (C3, C4, C5 at velocity 96) in the same file, dB. src/drumkit.cc.',
         '#pragma once', 'namespace pajupilli {',
         'constexpr int kDrumCalVels[4] = {%s};' % ', '.join(map(str, sc.VELS)),
         'constexpr int kDrumCalGridN = %d;' % len(KIT_GRID),
         'constexpr int kDrumCalGrid[kDrumCalGridN] = {%s};' % ', '.join(map(str, KIT_GRID)),
         'struct DrumCal { int kit, key; float target[4], own[kDrumCalGridN]; };', 'const DrumCal kDrumCal[] = {'] + L + ['};', '}  // namespace pajupilli']
    open(KIT_HEADER, 'w').write('\n'.join(H) + '\n')
    print('wrote %s (%d keys): build, then `kits --pj-only` to check' % (KIT_HEADER, len(L)), file=sys.stderr)


def kits_report(R, measure=0):
    T = kits_table(R, measure)
    what = ['loudest 400 ms', 'whole stroke (energy)', 'peak'][measure]
    L = ['# The drum kits against the SoundFonts, stroke by stroke', '',
         'tools/gmcompare.py kits. Each General MIDI drum key at velocities %s on the Standard, Jazz and Brush kits '
         '(the drum channel\'s program 1, 33, 41), on pajupilli and on %s. Measure: %s, K-weighted (BS.1770), '
         'each synth\'s stroke against its own piano (C3, C4, C5 at velocity 96 in the same file). The numbers are pajupilli '
         'less the SoundFonts\' average, dB: positive = pajupilli\'s piece is louder against the piano than the fonts\'. '
         'The Brush kit\'s toms, hi-hat and cymbals are compared with the Standard kit\'s average less %g dB (the fonts\' Brush '
         'kits play the stick\'s samples there). '
         '(range): how far the fonts lie apart at velocity 96.' % (', '.join(map(str, sc.VELS)), ', '.join(REFS), what, -KIT_BRUSH_DB), '']
    for kit, _ in KITS:
        if kit not in T: continue
        L += ['## %s kit' % kit.capitalize(), '']
        L += ['| group | mean | ' + ' | '.join('v%d' % v for v in sc.VELS) + ' |', '|---|---|' + '---|' * len(sc.VELS)]
        for g, keys in DRUM_GROUPS:
            c = [[T[kit][k][v][0] for k in keys if v in T[kit][k]] for v in sc.VELS]
            al = [x for cc in c for x in cc]
            if al: L.append('| %s | %+.1f | %s |' % (g, np.mean(al), ' | '.join('%+.1f' % np.mean(cc) if cc else '' for cc in c)))
        L += ['', '| key | drum | mean | ' + ' | '.join('v%d' % v for v in sc.VELS) + ' | (range) | re piano v96: pajupilli / fonts |',
              '|---|---|---|' + '---|' * len(sc.VELS) + '---|---|']
        for k in range(35, 82):
            row = T[kit][k]
            if not row: L.append('| %d | %s | silent |' % (k, DRUM_NAMES[k - 35]) + ' |' * (len(sc.VELS) + 2)); continue
            L.append('| %d | %s | %+.1f | %s | %s | %s |' % (
                k, DRUM_NAMES[k - 35], np.mean([r[0] for r in row.values()]),
                ' | '.join('%+.1f' % row[v][0] if v in row else '' for v in sc.VELS),
                '%.1f' % row[96][1] if 96 in row else '', '%.1f / %.1f' % (row[96][2], row[96][3]) if 96 in row else ''))
        L.append('')
    open(KIT_REPORT, 'w').write('\n'.join(L) + '\n')
    print('wrote ' + KIT_REPORT, file=sys.stderr)


# ---------------------------------------------------------------------------------------------
# The melodic programs' levels against the piano (research/gm_levels.md)

LV_SLOT, LV_HOLD = 3.5, 2.0
LV_DATA = os.path.join(os.path.dirname(DATA), 'levels.json')
LV_REPORT = os.path.join(ROOT, 'research', 'gm_levels.md')


def lv_keys(p):
    """Five keys over the middle of the program's GM range."""
    lo, hi = sc.gm_range(p)
    return sorted({int(round(lo + f * (hi - lo))) for f in (0.2, 0.35, 0.5, 0.65, 0.8)})


def write_lv(path, p, keys=None, vels=None, slot=LV_SLOT, hold=LV_HOLD):
    """The piano's yardstick (channel 1), then the program (channel 2) at keys x vels (lv_keys x sc.VELS)."""
    tpq = 480; tick = lambda s: int(round(s * 2 * tpq))
    lay = [(0, k, 96) for k in KIT_ANCHOR] + [(1, k, v) for k in (keys or lv_keys(p)) for v in (vels or sc.VELS)]
    lay = [(ch, k, v, i * slot) for i, (ch, k, v) in enumerate(lay)]
    ev = [(0, bytes([0xC0, 0])), (0, bytes([0xC1, p]))]
    for ch, k, v, t in lay:
        ev.append((tick(t), bytes([0x90 | ch, k, v]))); ev.append((tick(t + hold), bytes([0x80 | ch, k, 0])))
    ev.sort(key=lambda e: (e[0], e[1][0] & 0xF0 == 0x90))
    body = bytearray(); last = 0
    for t, e in ev: body += sc._enc_vlq(t - last) + e; last = t
    body += sc._enc_vlq(tick(slot)) + b'\xff\x2f\x00'
    open(path, 'wb').write(b'MThd' + sc.struct.pack('>IHHH', 6, 0, 1, tpq) + b'MTrk' + sc.struct.pack('>I', len(body)) + bytes(body))
    return lay


def lv_levels(x, t, slot=LV_SLOT):
    """A note's loudest 400 ms (K-weighted), its energy over the slot, its peak (dB)."""
    seg = x[int(t * FS):int((t + slot - 0.05) * FS)]
    if len(seg) < FS: return None
    p = (sc.kweight(seg) ** 2).sum(axis=1)
    B, H = int(0.4 * FS), int(0.02 * FS)
    c = np.concatenate([[0], np.cumsum(p)]); idx = np.arange(0, len(p) - B, H)
    mom = float(sc.db(((c[idx + B] - c[idx]) / B).max()))
    return [sc.r1(mom), sc.r1(sc.db(p.sum() / FS)), sc.r1(20 * math.log10(max(float(np.abs(seg).max()), 1e-7)))]


def lv_one(p, name, kind, font, pjbin, workroot, env_extra=None, probe=None):
    """probe: (keys, vels, slot, hold); None: the levels probe."""
    keys, vels, slot, hold = probe or (None, None, LV_SLOT, LV_HOLD)
    work = os.path.join(workroot, 'lv%s_%03d_%s' % ('k' if probe else '', p, name.replace('/', '_'))); os.makedirs(work, exist_ok=True)
    try:
        mid = os.path.join(work, 'p.mid'); lay = write_lv(mid, p, keys, vels, slot, hold)
        wav = os.path.join(work, 'out.wav'); log = ''
        if kind == 'pj':
            r = subprocess.run([pjbin, '-director', 'off', '-play', mid, '-o', wav],
                               env=dict(os.environ, PJ_PLAY_VELMEDIAN='1000', PJ_THREADS='2', **(env_extra or {})),
                               capture_output=True, text=True)
            log = r.stdout + r.stderr
            if r.returncode != 0: raise RuntimeError(log[-2000:])
        elif kind == 'tim':
            sc.CACHE = work
            r = subprocess.run(['timidity', '-c', sc.timidity_cfg(SF[font]), '-s', str(FS), '-OwS2', '-EFreverb=d', '-EFchorus=d',
                                '-o', wav, mid], capture_output=True, text=True)
            if r.returncode != 0 or not os.path.exists(wav): raise RuntimeError((r.stdout + r.stderr)[-2000:])
        else:
            render_fluid(font, mid, wav)
        x = sc.load_wav(wav)
        m = re.search(r'peak [\d.]+: the whole song (-?[\d.]+) dB down', log)
        if m: x = x * 10 ** (-float(m.group(1)) / 20)
        anchor = []; S = {}
        for ch, k, v, t in lay:
            l = lv_levels(x, t, slot)
            if l is None: continue
            if ch == 0: anchor.append(l)
            else: S['%d/%d' % (k, v)] = l
        return p, name, dict(anchor=[sc.r1(np.mean([a[i] for a in anchor])) for i in range(3)], notes=S,
                             log=[l for l in log.splitlines() if 'channel' in l][:4])
    finally:
        shutil.rmtree(work, ignore_errors=True)


def cmd_levels(a):
    names, _ = pj_names(os.path.abspath(a.bin))
    progs = [p for p in parse_programs(a.programs, names) if p != 'drums']
    R = json.load(open(LV_DATA)) if os.path.exists(LV_DATA) else {}
    if not a.report_only:
        synths = [s for s in SYNTHS if s[0] == 'pajupilli' or s[0] in REFS]
        if a.pj_only: synths = synths[:1]
        with ProcessPoolExecutor(a.jobs) as ex:
            fut = [ex.submit(lv_one, p, n, kind, font, os.path.abspath(a.bin), a.work) for p in progs for n, kind, font in synths
                   if a.force or n == 'pajupilli' or n not in R.get(str(p), {})]
            for i, f in enumerate(as_completed(fut)):
                try:
                    p, name, res = f.result(); res['inst'] = names.get(p); R.setdefault(str(p), {})[name] = res
                except Exception:
                    print('FAILED\n' + traceback.format_exc(), file=sys.stderr); continue
                if i % 20 == 0: print('%d/%d' % (i, len(fut)), file=sys.stderr, flush=True)
        json.dump(R, open(LV_DATA, 'w'), indent=0)
    if a.emit: lv_emit(R)
    lv_report(R, a.measure)


def lv_rel(D, measure=0):
    """synth -> 'key/vel' -> the note against that synth's own piano (dB); silent notes left out."""
    return {n: {s: l[measure] - d['anchor'][measure] for s, l in d['notes'].items() if l[2] > -80} for n, d in D.items()}


def lv_table(R, measure=0):
    """program -> velocity -> (pajupilli less the fonts' mean, the fonts' spread (sd over the fonts of
    their key-mean), pajupilli, the fonts' mean, per-font), each averaged over the keys."""
    T = {}
    for ps, D in R.items():
        rel = lv_rel(D, measure)
        if 'pajupilli' not in rel: continue
        row = {}
        for v in sc.VELS:
            keys = [s for s in rel['pajupilli'] if s.endswith('/%d' % v)]
            pf = {n: [rel[n][s] for s in keys if s in rel.get(n, {})] for n in REFS}
            per = {n: float(np.mean(x)) for n, x in pf.items() if x}
            pj = [rel['pajupilli'][s] for s in keys]
            if len(per) < 2 or not pj: continue
            fm = float(np.mean(list(per.values())))
            row[v] = (float(np.mean(pj)) - fm, float(np.std(list(per.values()))), float(np.mean(pj)), fm, per)
        T[int(ps)] = row
    return T


def lv_report(R, measure=0):
    T = lv_table(R, measure)
    what = ['loudest 400 ms', 'whole note (energy)', 'peak'][measure]
    L = ['# The General MIDI programs\' levels against the SoundFonts', '',
         'tools/gmcompare.py levels. Each melodic program plays five keys over the middle of its GM range at velocities %s '
         '(held %g s), after a piano\'s C3, C4, C5 at velocity 96 in the same file. Measure: %s, K-weighted (BS.1770), each '
         'synth\'s note against its own piano, averaged over the keys. pajupilli less the mean of %s, dB: positive = pajupilli\'s '
         'instrument is louder against its piano than the fonts\'. sd: the spread of the four fonts (their standard deviation) at '
         'velocity 96. The dynamic range is v127 less v40.' % (', '.join(map(str, sc.VELS)), LV_HOLD, what, ', '.join(REFS)), '']
    allv = {v: [T[p][v][0] for p in T if v in T[p]] for v in sc.VELS}
    rng = [(T[p][127][2] - T[p][40][2], T[p][127][3] - T[p][40][3]) for p in T if 127 in T[p] and 40 in T[p]]
    sd = [T[p][96][1] for p in T if 96 in T[p]]
    L += ['## Summary', '',
          '| | v40 | v64 | v96 | v127 |', '|---|---|---|---|---|',
          '| mean | ' + ' | '.join('%+.1f' % np.mean(allv[v]) for v in sc.VELS) + ' |',
          '| mean abs | ' + ' | '.join('%.1f' % np.mean(np.abs(allv[v])) for v in sc.VELS) + ' |',
          '| within 3 dB | ' + ' | '.join('%d %%' % round(100 * np.mean(np.abs(allv[v]) <= 3)) for v in sc.VELS) + ' |', '',
          'The fonts\' own spread at v96: median sd %.1f dB. Dynamic range v40-v127: pajupilli median %.1f dB, fonts %.1f dB.' % (
              np.median(sd), np.median([r[0] for r in rng]), np.median([r[1] for r in rng])), '']
    for gi in range(16):
        L += ['## %s' % ['Piano', 'Chromatic percussion', 'Organ', 'Guitar', 'Bass', 'Strings', 'Ensemble', 'Brass', 'Reed', 'Pipe',
                         'Synth lead', 'Synth pad', 'Synth effects', 'Ethnic', 'Percussive', 'Sound effects'][gi], '',
              '| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |',
              '|---|---|---|---|---|---|---|---|---|---|---|']
        for p in range(gi * 8, gi * 8 + 8):
            row = T.get(p)
            if not row or 96 not in row: L.append('| %d | %s | | missing |' % (p + 1, GM[p]) + ' |' * 7); continue
            inst = next((d.get('inst') for d in R[str(p)].values() if d.get('inst')), '')
            r = (row[127][2] - row[40][2], row[127][3] - row[40][3]) if 127 in row and 40 in row else None
            L.append('| %d | %s | %s | %+.1f | %s | %.1f | %s | %.1f / %.1f |' % (
                p + 1, GM[p], inst, np.mean([x[0] for x in row.values()]),
                ' | '.join('%+.1f' % row[v][0] if v in row else '' for v in sc.VELS), row[96][1],
                '%.1f / %.1f' % r if r else '', row[96][2], row[96][3]))
        L.append('')
    open(LV_REPORT, 'w').write('\n'.join(L) + '\n')
    print('wrote ' + LV_REPORT, file=sys.stderr)


LV_HEADER = os.path.join(ROOT, 'src', 'gmlevels_data.h')
LV_W = {40: 0.5, 64: 1.0, 96: 1.0, 127: 0.5}  # (the level matched: the velocities a file plays most, weighted)
LV_ANCHOR = 'grand-piano'  # (each synth's yardstick: not moved)
# Instruments whose level is set against another's in their own code: they take that one's.
# (The brass section: a unison key 2 dB over one trumpet, src/brasssect.cc B.gain, as the fonts have it
# (1.6 dB over five keys); pajupilli's trumpet and section are each uneven key to key by +-5 dB, and
# matched apart the section came out 5 dB under a trumpet at G4: tools/test.py brass_section.)
LV_TIED = {'brass-section': 'trumpet'}


def lv_emit(R, gain=0.9):
    """src/gmlevels_data.h: per instrument, the trim (dB) on top of src/levels_data.h's that brings it to the
    SoundFonts' mean against the piano. The numbers measured are with the present table in force: what is
    still missed is added to it (times `gain`, within +-15 dB)."""
    T = lv_table(R); old = {}
    if os.path.exists(LV_HEADER):
        for nm, db in re.findall(r'\{"([^"]+)", (-?[\d.]+)f\}', open(LV_HEADER).read()): old[nm] = float(db)
    L = []; new = dict(old)
    for p in sorted(T):
        row = T[p]; inst = next((d.get('inst') for d in R[str(p)].values() if d.get('inst')), None)
        if not inst or inst == LV_ANCHOR or not row: continue
        w = [(LV_W[v], row[v][0]) for v in row]
        miss = sum(a * b for a, b in w) / sum(a for a, _ in w)
        new[inst] = round(float(np.clip(old.get(inst, 0) - gain * miss, -15, 15)), 1)
    for a, b in LV_TIED.items():
        if b in new: new[a] = new[b]
    for p in range(128):
        inst = next((d.get('inst') for d in R.get(str(p), {}).values() if d.get('inst')), None)
        if inst in new: L.append('    {"%s", %.1ff},  // GM %d %s' % (inst, new[inst], p + 1, GM[p]))
    H = ['// Generated by tools/gmcompare.py levels --emit: do not edit.',
         '// Per instrument, the trim (dB) on top of src/levels_data.h\'s that puts it where the SoundFonts',
         '// (FluidR3, TimGM6mb, Jnsgm2, SGM) put that General MIDI program against their piano: its loudest 400 ms,',
         '// K-weighted, five keys at velocities 40, 64, 96, 127 (weights 1, 2, 2, 1), against the piano\'s C3, C4, C5',
         '// at velocity 96 in the same file. research/gm_levels.md. PJ_NOGMLEVELS: none.',
         '#pragma once', 'namespace pajupilli {', 'struct GmLevel { const char* name; float db; };',
         'constexpr GmLevel kGmLevels[] = {'] + L + ['};', '}  // namespace pajupilli']
    open(LV_HEADER, 'w').write('\n'.join(H) + '\n')
    print('wrote %s (%d instruments): build, then `levels --pj-only` to check' % (LV_HEADER, len(L)), file=sys.stderr)


# ---------------------------------------------------------------------------------------------
# Each key's level against the instrument's others (research/gm_keys.md, src/gmkeys_data.h)

KY_VELS = (64, 100)  # (not the profile probe's: the table is not fitted to the notes the distance is scored on)
KY_SLOT, KY_HOLD = 2.2, 1.2
KY_DATA = os.path.join(os.path.dirname(DATA), 'keys.json')
KY_HEADER = os.path.join(ROOT, 'src', 'gmkeys_data.h')
KY_REPORT = os.path.join(ROOT, 'research', 'gm_keys.md')
KY_SKIP = {'grand-piano'}  # (each synth's yardstick)


def ky_keys(p):
    """Every key of the program's GM range."""
    lo, hi = sc.gm_range(p)
    return list(range(lo, hi + 1))


def ky_rel(D, names):
    """name -> key -> the key's level against the piano (dB, the mean over KY_VELS); silent keys left out."""
    rel = lv_rel(D)
    out = {}
    for n in names:
        if n not in rel: continue
        by = {}
        for s, v in rel[n].items():
            k, _ = map(int, s.split('/')); by.setdefault(k, []).append(v)
        out[n] = {k: float(np.mean(v)) for k, v in by.items() if len(v) == len(KY_VELS)}
    return out


def ky_shape(R, p):
    """key -> (pajupilli's level less the fonts' mean, each less its own mean over the keys both have:
    the keyboard's shape, not the instrument's level), and the fonts' sd per key; None if too few keys."""
    D = R.get(str(p))
    if not D or 'pajupilli' not in D: return None
    rel = ky_rel(D, ['pajupilli'] + REFS)
    keys = [k for k in rel.get('pajupilli', {}) if sum(k in rel.get(n, {}) for n in REFS) >= 2]
    if len(keys) < 6: return None
    fm = {k: float(np.mean([rel[n][k] for n in REFS if k in rel.get(n, {})])) for k in keys}
    fs = {k: float(np.std([rel[n][k] for n in REFS if k in rel.get(n, {})])) for k in keys}
    pm = np.mean([rel['pajupilli'][k] for k in keys]); m = np.mean([fm[k] for k in keys])
    return {k: (rel['pajupilli'][k] - pm - (fm[k] - m), fs[k]) for k in sorted(keys)}


def ky_stats(sh):
    """The shape's rms (dB) and its scatter after a smooth trend (a quadratic over the keys) is taken off."""
    k = np.array(sorted(sh)); d = np.array([sh[x][0] for x in k])
    c = np.polyfit(k, d, 2) if len(k) > 3 else [0, 0, float(np.mean(d))]
    return float(np.sqrt(np.mean(d ** 2))), float(np.std(d - np.polyval(c, k)))


def cmd_keys(a):
    names, _ = pj_names(os.path.abspath(a.bin))
    progs = [p for p in parse_programs(a.programs, names) if p != 'drums']
    R = json.load(open(KY_DATA)) if os.path.exists(KY_DATA) else {}
    if not a.report_only:
        synths = [s for s in SYNTHS if s[0] == 'pajupilli' or s[0] in REFS]
        if a.pj_only: synths = synths[:1]
        with ProcessPoolExecutor(a.jobs) as ex:
            fut = [ex.submit(lv_one, p, n, kind, font, os.path.abspath(a.bin), a.work, None, (ky_keys(p), KY_VELS, KY_SLOT, KY_HOLD))
                   for p in progs for n, kind, font in synths if a.force or n == 'pajupilli' or n not in R.get(str(p), {})]
            for i, f in enumerate(as_completed(fut)):
                try:
                    p, name, res = f.result(); res['inst'] = names.get(p); R.setdefault(str(p), {})[name] = res
                except Exception:
                    print('FAILED\n' + traceback.format_exc(), file=sys.stderr); continue
                if i % 20 == 0: print('%d/%d' % (i, len(fut)), file=sys.stderr, flush=True)
        json.dump(R, open(KY_DATA, 'w'), indent=0)
    if a.emit: ky_emit(R)
    ky_report(R)


def ky_emit(R, gain=0.8):
    """src/gmkeys_data.h: per instrument and key, what is taken off (dB) to bring its keyboard to the fonts' shape;
    the numbers measured with the present table in force, what is still missed added to it (times gain, within
    +-10 dB). Smoothed over three keys (a key's own reading is a few tenths of a dB from the room)."""
    old = {}
    if os.path.exists(KY_HEADER):
        for nm, ks, ds in re.findall(r'\{"([^"]+)", \d+, \{([^}]*)\}, \{([^}]*)\}\}', open(KY_HEADER).read()):
            if nm: old[nm] = dict(zip(map(int, ks.split(',')), (float(x.strip().rstrip('f')) for x in ds.split(','))))
    rows = []
    for p in range(128):
        D = R.get(str(p)); sh = ky_shape(R, p)
        inst = next((d.get('inst') for d in (D or {}).values() if d.get('inst')), None)
        if not sh or not inst or inst in KY_SKIP: continue
        k = sorted(sh); d = np.array([sh[x][0] for x in k])
        ds = np.convolve(np.pad(d, 1, mode='edge'), np.ones(3) / 3, 'valid')
        o = old.get(inst, {})
        new = [float(np.clip(o.get(x, 0) - gain * y, -10, 10)) for x, y in zip(k, ds)]
        rows.append((p, inst, k, new))
    L = ['    {"%s", %d, {%s}, {%s}},  // GM %d %s' % (inst, len(k), ', '.join(map(str, k)), ', '.join('%.1ff' % x for x in new), p + 1, GM[p])
         for p, inst, k, new in rows]
    H = ['// Generated by tools/gmcompare.py keys --emit: do not edit.',
         '// Per instrument, each key\'s level (dB) against its others, so the keyboard has the shape the SoundFonts',
         '// (FluidR3, TimGM6mb, Jnsgm2, SGM) give its General MIDI program: every key, velocities 64 and 100, its',
         '// loudest 400 ms, K-weighted, against a piano\'s C3, C4, C5 in the same file. research/gm_keys.md.',
         '// (src/instruments.cc gmKeyDb: a gain on the note, or the stroke that sounds so loud). PJ_NOGMKEYS: none.',
         '#pragma once', 'namespace pajupilli {', 'struct GmKeys { const char* name; int n; int key[88]; float db[88]; };',
         'const GmKeys kGmKeys[] = {'] + L + ['};', '}  // namespace pajupilli']
    open(KY_HEADER, 'w').write('\n'.join(H) + '\n')
    print('wrote %s (%d instruments)' % (KY_HEADER, len(L)), file=sys.stderr)


def ky_report(R):
    L = ['# Each key\'s level against the SoundFonts', '',
         'tools/gmcompare.py keys. Every key of each program\'s GM range at velocities %s (held %g s), after a piano\'s C3, C4, '
         'C5 at velocity 96 in the same file; loudest 400 ms, K-weighted, against the synth\'s own piano. **Shape**: pajupilli\'s '
         'level per key less the fonts\' mean, each less its own mean over the keys (the instrument\'s level is '
         'research/gm_levels.md\'s): its rms over the keys, and the **scatter** left after a smooth trend (a quadratic) is taken '
         'off. **Fonts sd**: the four fonts\' spread per key, its median.' % (', '.join(map(str, KY_VELS)), KY_HOLD), '',
         '| GM | program | pajupilli | shape rms (dB) | scatter (dB) | fonts sd (dB) | keys |', '|---|---|---|---|---|---|---|']
    S = []
    for p in range(128):
        sh = ky_shape(R, p)
        inst = next((d.get('inst') for d in R.get(str(p), {}).values() if d.get('inst')), '')
        if not sh: L.append('| %d | %s | %s | | | | |' % (p + 1, GM[p], inst)); continue
        rms, scat = ky_stats(sh); S.append((rms, scat))
        L.append('| %d | %s | %s | %.1f | %.1f | %.1f | %d |' % (p + 1, GM[p], inst, rms, scat, np.median([v[1] for v in sh.values()]), len(sh)))
    if S:
        L[2:2] = ['Median over the programs: shape rms %.1f dB, scatter %.1f dB.' % (np.median([s[0] for s in S]), np.median([s[1] for s in S])), '']
    open(KY_REPORT, 'w').write('\n'.join(L) + '\n')
    print('wrote %s; median shape rms %.2f, scatter %.2f dB' % (KY_REPORT, np.median([s[0] for s in S]), np.median([s[1] for s in S])), file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('cmd', choices=['run', 'report', 'kits', 'levels', 'keys'])
    ap.add_argument('--bin', default=os.path.join(ROOT, 'pajupilli'))
    ap.add_argument('--programs', default='0-127,drums', help='GM programs 0-127 (0-based), ranges, pajupilli names, "drums"')
    ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--force', action='store_true', help='redo instruments that already have results')
    ap.add_argument('--work', default=os.path.expanduser('~/.cache/gmcompare'), help='renders (deleted per instrument)')
    ap.add_argument('--no-zimt', action='store_true')
    ap.add_argument('--head', default=os.path.join(ROOT, 'research', 'gm_compare', 'head.md'))
    ap.add_argument('--patterns', default=os.path.join(ROOT, 'research', 'gm_compare', 'patterns.md'))
    ap.add_argument('--tail', default=os.path.join(ROOT, 'research', 'gm_compare', 'methods.md'))
    ap.add_argument('--pj-only', action='store_true', help='kits: render pajupilli again, keep the SoundFonts\' numbers')
    ap.add_argument('--emit', action='store_true', help='kits: measure pajupilli uncalibrated and write src/drumcal_data.h; levels: with --report-only, add what is missed to src/gmlevels_data.h')
    ap.add_argument('--refine', action='store_true', help='kits: after --emit, a build and a `kits --pj-only`: take what is still missed off the targets, write the header again')
    ap.add_argument('--report-only', action='store_true', help='kits: the report from the numbers at hand')
    ap.add_argument('--measure', type=int, default=0, help='kits: 0 the loudest 400 ms, 1 the whole stroke, 2 the peak')
    a = ap.parse_args()
    if a.cmd == 'kits': return cmd_kits(a)
    if a.cmd == 'levels': return cmd_levels(a)
    if a.cmd == 'keys': return cmd_keys(a)
    cmd_run(a) if a.cmd == 'run' else cmd_report(a)


if __name__ == '__main__':
    main()
