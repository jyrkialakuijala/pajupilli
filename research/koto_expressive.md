# The koto played expressively: research notes for the player

Status: 2026-11-26. Built from §6: 1 (the ji planned for the piece, `src/kotoplan.cc`; the tuning by
ear, the organ's jyrki-spectral, in `ModalPlayer::kotoNote`), 2 (the press before the pluck, the
lean), and the pressing finger's damping (each partial by its swing at the finger, the owner's idea).
The tuner (`ModalPlayer::kotoTune`): the ji set string against string by the lowest-order partial
coincidence up to 4 (so the koto's intervals emerge: pure fifths, fourths, octaves; Pythagorean
thirds; D-hirajōshi's semitones by sense at 90 c, D 0, G -2, A +2, Bb -8, Eb -10 c before centring).
Tuned by whatever rang at each string's first pluck instead, thirds came out just (-14 c).
The body's modes under 100 Hz turned down by ear (`PJ_KOTO_LOW` 0.35). The pluck "10 dB too
loud" (the owner's ear): the body was fed at the fixed bridge, 3 cm from the tsume, so the pull's
whole slow force thumped it (its modes under the note louder than the note for 30 ms); fed at the
ji, the far end, the thump is 5-20 dB lower, the attack's excess over the ring 8-10 dB, not
10-16 (`PJ_KOTO_JI_END`); a moved ji keeps the tsume 3 cm from the fixed bridge (it slid to 3/r^2
cm); the level trim +2.0 dB to the same loudness.
§6.4 the right hand (the owner's account, 2026-11-27: the thumb's tsume furthest from the end,
then the index, then the middle; tsume pull or push, at their own angles; the left hand's bare
finger softer on the bass strings; three-, two- or one-finger playing, the index pulling through):
`finger()` in `src/kotoplan.cc`, `ModalPlayer::kotoStroke`. ABBA2's part: thumb 49 %, index 21 %,
middle 28 %, the left hand 2 %.
Not yet: 3 (ato-oshi, oshi-hanashi, yuri, hiki-iro, tsuki-iro read from the music), 4, 5, 6. Complements `banjo_koto.md` (the
strings, the body, the techniques' mechanics, §7–8). This file covers how a player *uses* those
techniques, what the model does today, and what to build.

**How numbers are marked.** As in `banjo_koto.md`: a reference key [Ando89] means read in the primary
text; **[D]** derived; **[G]** my estimate; **[U]** from a blog, a vendor or a secondary source.
Little about koto performance is measured. Almost every timing below is [G] or [U]: the owner's ear decides.

## 1. What the model does today (src/modalinst.cc, `kKoto`)

- 13 strings, hirajōshi on D, **equal-tempered** (`hz(t)` of MIDI 50 55 57 58 62 63 67 69 70 74 75 79 81).
- A key takes the string tuned at it or 1–2 semitones below. A note above the string is pressed (oshide):
  the pitch starts at 80 % of the bend and reaches it 60 ms after the pluck. That is an audible scoop of
  20–40 cents on every pressed note.
- A key out of reach (C♯, F♯) retunes the nearest string instantly: its ji is moved, with no sound of it.
- One kind of pluck: 6.25 × amp, rise 2 ms, at 3 cm. The re-pluck stops part of a ringing string (0.6).
- Key release: the string rings on (no damping). A pressed string stays pressed.
- Pitch wheel: ±1 semitone on the held strings. No modulation (yuri), no finger roles, no rest stroke,
  no chords or glissandi as such, no pick noise.

The koto parts in the owner's MIDI collection (36 files with GM 108) are nearly all Western pop or game
music in arbitrary keys: on a fixed D hirajōshi most of their notes are pressed or moved strings.

## 2. Tuning: how players really tune

- **Ando 1989**, six professional players (five lecturers at Tokyo National University of Fine Arts and
  Music), D-hira-jōshi, each recorded three times, pitch-detected [Ando89]:
  - The players tune by **perfect fifths, fourths and octaves** (8 octave transfers); only the two
    semitones (strings 3–4 and 5–6) are set "by musical sense".
  - The semitones average close to the **Pythagorean 90 cents**: "quite narrow compared with … equal
    temperament (100 cents) and just intonation (112 cents)". They scatter most between players;
    one player "purposely altered the semi-tone intervals to provide different nuances".
  - **Octaves stretch upward** in the high strings, as on a piano. Fifths come out near pure, one
    "distorted" fifth wider (fixed in performance by nudging a ji); fourths scatter and lean narrow.
  - The standard deviation between players is largest at the semitones.
  - Oshide and hikide (pressing and pulling) "cause the ji to slip"; humidity and temperature move pitch.
- **Choosing the tuning.** A player sets the 13 ji for the piece: a named chōshi (hira, kumoi, kokin,
  nogi, gaku, iwato …; hirajōshi 40 % of the 220 classical pieces in Sōkyoku Jiuta Taikei [Ando89]), or for
  Western-style music a diatonic set (e.g. "Flowers will bloom": G A B♭ C D E F G A B♭ C D E [Kawai]).
  Most classical pieces **modulate by moving ji mid-piece**. A note outside the tuning is pressed;
  the reach is a whole tone, a minor third only with effort on tetron strings [Kawai].
- **Pressed intonation.** Players over-press the semitone mark (ヲ) and under-press the whole-tone (オ)
  [U, ameblo]: the two pressed intervals lean toward each other.

## 3. The left hand

| Technique | What happens | Pitch contour | Timing | Notes |
|---|---|---|---|---|
| **Oshide** (押し手) | press 10–15 cm left of the ji, **before** the pluck | the note starts at pitch | pressed in advance; "no passing tone" [U, yuyu, mm5] | ヲ ≈ +1, オ ≈ +2 semitones; held while the note sounds |
| **Ato-oshi** (後押し) | pluck, then press, carrying the raised pitch into the next note | a rising glide on the ringing string, **no second pluck** | after the pluck; "the resonance is what matters" [U, yuyu] | the passing pitch *is* the effect |
| **Oshi-hanashi** (押し放し) | pluck pressed, then let go smoothly, finger still on the string | a falling glide back to the open pitch | released "スッ" (smoothly) [U, ameblo] | small, delicate, needs a quiet room [Kawai]; "short trailing tones" in modern pieces [Senzoku] |
| **Tsuki-iro** (突き色) | push just after the pluck and release at once | a quick up-and-back ("suddenly goes up and down") | "immediately after"; a little late sounds unnatural [U, ameblo] | a defining koto sound; Rokudan [Kawai] |
| **Yuri-iro** (揺り色) | rock the string left of the ji (or pulse a press) | vibrato, **above** the open pitch (pressing only raises tension) | starts after the pluck; length "at the player's discretion" | no published rate or depth; added where unwritten [Kawai] |
| **Hiki-iro** (引き色) | pinch left of the ji and pull toward it | a small fall in the tail | after the pluck, the last part of the note | about a quarter semitone, "just audible" [U, mm5] |
| **Keshi / mute** | finger on the string over the ji | muffled, quieter; buzzing if touched lightly | with the pluck | modern [Kawai, yuyu] |

- **Yuri is one-sided.** A press can only add tension, so the pitch rocks between the base and
  something above it. It does not swing either side as a violin vibrato does [D from the mechanics].
  Proposal: 4–6 Hz, 15–40 cents peak, starting 0.15–0.3 s after the pluck, 2–5 cycles, fading [G].
- **Ato-oshi vs oshide is a matter of timing alone**: the same press, before or after the pluck. In a
  MIDI file the difference shows as a rise of 1–2 semitones reachable on the string just played.

## 4. The right hand

- **Finger roles** [Kawai, Stanford, GK]: the thumb carries the melody. Index and middle play the
  chords and ornaments (awase-zume, kaki-zume, waren).
- **Stroke direction and tone** [Stanford]: the thumb strikes upward (away), index and middle downward
  (toward the player). The downward stroke is stronger: the middle finger's D4 (ff) is richer in partials
  than the thumb's. The thumb from underneath (kozume, sukui) is softer still.
- **Rest stroke**: each stroke comes to rest on the next string ([CS11] in `banjo_koto.md` §7.1), which
  silences that string if it was ringing.
- **Two-string figures** [GK, mm5, K]:
  - Awase-zume: thumb + middle together, usually an octave (or a fifth).
  - Kaki-zume: middle finger across two adjacent strings in one stroke, low to high in Ikuta-ryū
    (high to low in Yamada) [U, K]. It sounds as one event: a gap of 0.03 s is almost inaudible, 0.07 s
    sounds broken [U, mm5].
  - Wari-zume: index then middle on the adjacent pair ("jan-jan"), sources disagree [U].
  - Oshi-awase: one string pressed up to the pitch of its neighbour, both plucked together, a few ms apart [U, mm5].
- **Glissandi and noise**:
  - Hiki-ren: middle finger sweeps the strings low to high. Nagashi-zume: thumb high to low, slower.
  - Ura-ren (sararin): tremolo on the top string, a sweep down, two or three strings to finish [GK, mm5].
  - Chirashi-zume / suri-zume: the side or back of the picks scraped along strings: noise, little pitch.
  - Sukui-zume: a scoop with the pick's back, softer, a different attack.
- **Pick vs string** [KAGUYA press sheet]: a commercial koto library starts the string "a slight delay"
  after the pick's sound, adjustable 0–100 ms. The ivory corner meets the string audibly before it slips.

## 5. Time

- Danmono: each dan 52 measures of 2/4 [NDL]. The tempo accelerates through the piece and slows into the
  close (jo-ha-kyū) [U, secondary sources only; no measured curve found]. Rokudan is played with
  "lingering resonance and pauses" [Senzoku].
- Ma: the silence between events is shaped by feel, not counted [U].
- This belongs to the director (Director.md), not the instrument; GM koto parts in pop files are on the grid.

## 6. What to build, in order of what is heard

1. **Tune the koto for the piece.** Before playing, read the part's pitches (by phrase or section) and set the
   13 ji so most notes are open strings: the chōshi or diatonic set covering the most notes, pressed notes
   only for the rest. Retune between phrases when the harmony moves, as players move ji, with a
   silent bridge move only where the string is quiet. Tune by fifths/fourths/octaves (semitones ≈ 90 c,
   a slight stretch up high) as Ando measured; a knob for equal temperament in an ensemble that needs it.
2. **Oshide before the pluck.** Press ahead when the hand has time (the previous note on that string, or the
   left hand free, ≳ 100 ms before [G]); the note then starts at pitch, with a small settle (≤ 10 c) and the
   ヲ/オ lean (semitone +5 c, whole tone −10 c [G]). Only a hurried press scoops into the note.
3. **The left hand's inflections, read from the music.**
   - Ato-oshi: a note 1–2 semitones above the previous one, reachable on that string, legato, soft
     (as the sitar's meend reads it), within ~0.6 s: the press, no new pluck.
   - Oshi-hanashi: down from a pressed note to the same string's open pitch: the release, no new pluck.
   - Yuri: on long notes (≳ 0.6 s), at phrase ends, one-sided vibrato (§3); the modulation wheel drives it.
   - Hiki-iro, tsuki-iro: occasionally, on long final notes and on strong single notes [G].
   - A pressed string let go when the hand is needed elsewhere: the pitch falls in the tail unless it is damped.
4. **The right hand.** Thumb for the melody (upward, softer, rounder), index/middle downward (brighter,
   stronger) for chord tones and repeated notes; the rest stroke stopping the next string; octave pairs
   ≲ 30 ms as awase-zume, adjacent pairs as kaki-zume (one stroke, ~10–25 ms between [G]).
5. **The tsume's sound**: a short ivory click as the corner meets the string, the string's release a few ms
   later (rise 2 → 3–8 ms by velocity [G]); scrapes and sweeps (chirashi, sararin) for fast runs and FX.
6. Later: the jo-ha-kyū tempo shape for danmono-like pieces (the director).

## References

[Ando89] M. Ando, "Koto scales and tuning", J. Acoust. Soc. Jpn. (E) 10, 5, 279–287 (1989).
https://www.jstage.jst.go.jp/article/ast1980/10/5/10_5_279/_pdf/-char/en
[Kawai] Kawai Shamisen & Koto shop blog: "How many sounds koto can make?", "Yuri-iro, mute, oshi-hanashi",
"Koto's special chord", "Oshide". https://kawai-samisen-koto-en.blogspot.com/ [U]
[Stanford] Gagaku at Stanford, "Koto". https://gagaku.stanford.edu/en/strings/koto (gakusō practice)
[NDL] National Diet Library, "Koto melodies". https://www.ndl.go.jp/kaleido/e/entry/24/1.html
[Senzoku] Senzoku Gakuen, traditional music: koto. https://es01.senzoku-online.jp/traditional/English/contents/01-koto.html
[GK] Global Koto Music, "Writing koto music". https://globalkotomusic.com/writing-music/ [U]
[AF] A. Freedman, "Composing for Japanese instruments". https://adrianfreedman.com/composing-japanese-instruments/ [U]
[KAGUYA] Premier Sound Factory, "Princess Koto KAGUYA" press sheet. https://premiersoundfactory.jp/pressdoc/KAGUYA-PRESS_En.pdf [U]
[K] koto-shami.info, 箏の基本奏法. [U]
[ameblo] 桜庵の箏日記, 左手の手法. https://ameblo.jp/hagata47104/entry-12732620664.html [U]
[yuyu] 実はこんなにあった！？左手の奏法. https://yuyu-koto.hatenablog.com/entry/2019/12/10/001127 [U]
[mm5] MM5 Music Studio, KOTO NATIONで箏の打ち込み. https://mm5musics.com/koto/ [U]
[JUCR] K. Pornprasit, "Playing techniques of new pieces of koto music in Japanese urban culture", J. Urban
Culture Research. (modern techniques: harmonics, uji-zume, palm strikes, playing left of the ji)
