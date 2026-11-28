# Winds moved as they are played (2026-11-28)

The owner: "the minimum modulation that these instruments can do is that oboe or clarinet can do a small
directional spiral in the room where then the sound is projected more interestingly, flute player can sway a
bit, trumpet can use up and down direction for changing the ability to project"; "shakuhachi blowing technique is
delicate/difficult -- it is likely better moved only up and down like trumpet"; "recorder can be moved freely in
space like oboe and clarinet".

## What was built

- **Held into the room** (`Tube::aimOn`, as the shakuhachi was since 2026-11-27): each opening (the
  embouchure, every tone hole, the bell or foot) radiates from its own place and direction, and the room gives
  each its own paths. Now also the clarinet, the oboe, the cor anglais, the recorder, the flutes (flute, flute-c,
  piccolo, Baroque flute). `PJ_HELD_AIM=0`: the shakuhachi alone, as before.
  - The cost was no concern: the clarinet's file rendered in 20.8 s aimed, against 23.3 s as a point (noise).
- **The motion** (`src/heldaim.h`, driven by `Player::aim` and `ReedPlayer::aim`):
  - **Every kind:** lifted towards the hall as the playing grows loud, on an accent and in the high register;
    bowed into a phrase's end. Between phrases it rests lower. Two lags: the lift in ~0.3 s, the side in ~0.6 s.
  - **Up and down only** (the shakuhachi): its side turns per phrase and its sway are gone.
  - **Free** (clarinet, oboe, cor anglais, recorder): held 40° below the horizontal, the bell circling in a
    small ellipse (wider than tall). Its radius grows over a long note from 1° to 5°, a spiral outwards, and
    settles back between notes. Each phrase gets its own speed (0.17–0.28 turns a second) and sense.
  - **Transverse** (the flutes): a new geometry in `Tube::setPose` (`setTransverse`). The axis runs to the
    player's right, the foot 8° below the lips, the holes looking up and a little out. The body sways (the flute
    and the face with it), 2°, growing to 5° over a long note, at 0.15–0.25 Hz. Its lift is a quarter of the
    others'.
  - `PJ_AIM` scales all of it (0: held still).
- **The trumpet's bell** (`HornPlayer`, `PJ_TP_AIM`): raised and lowered as the playing asks:
  - 15° below the listener as usual, rising 36° per unit of level, 4° on an accent, 3° in the high octave;
  - lowered 10° into a phrase's end; resting at 30° between phrases; a slow drift of ±1.5°.
  - Heard as a shelf above 1.5 kHz (the bell's beaming): −`PJ_TP_BEAM` (6) dB × (angle / 30°)², against the
    usual angle. First tried as −20 dB × (1 − cos): near the axis that hardly moves (±0.2 dB).
- **[M]** The long-note test file (`research/gm_compare/work/panflute-long/long-<instrument>.mid`), moving against
  aimed but held still (`PJ_AIM=0`): the R–L balance per 0.25 s window moves by clarinet 1.3 dB rms (2.5 dB at the
  90th percentile), oboe 1.0 (2.1), recorder 0.9 (1.4), flute 0.6 (0.8). The level changes by −1.3 dB (clarinet)
  and −1.0 dB (oboe), from the lift. The trumpet, above 3 kHz: −1.0 dB at velocity 30, +0.9 dB at 110; +1.5 dB in
  the test file's busy passage.
- **Tests:** `shakuhachi_aim` passes with the lift alone (R–L moved +0.0, +3.4, −1.6 dB by phrase, through the
  room's paths); its text now says up and down. `brass` passes.
- **Listening:** `long-<instrument>-moving.wav` against `-still.wav` (the motion and the aiming off; the trumpet's
  bell still).

## Open

- The aiming itself (sounding from its own places, not as a point) changes the level a little against before,
  and the recorder's and flute's balance between their notes. It was not recalibrated.
- The brass section, the saxophones and the bassoon are not moved. Neither is the pan flute, whose
  tilting forward and back is in its own backlog.
