# Why players love the Hammond B-3 and its Leslie, and what the model must keep

Status: 2026-11-16. Research and requirements; nothing is built. This paper sits on top of
`research/hammond_b3.md` (the organ: generator, contacts, percussion, scanner, AO-28 preamp, model
plan in its §13) and `research/leslie.md` (the 122 speaker: rotors, acoustics, amplifier, model plan
in its §12). It does not repeat their physics. It asks which of their mechanisms carry the
instrument's emotional pull, turns those into requirements and tests, and lists what in the two
plans must change (§6).

Marks: **[paper]** a paper or primary document in §9; **[web]** interviews, books, magazines,
forums, makers' pages in §9: weaker as physics, but here the musicians' own words are the primary
evidence of what they feel; **[web, snippet]** a page that could not be fetched (HTTP 403 or
timeout) and is known only from a search engine's summary of it: weakest; **[memory]** general
knowledge, to check; **[calc]** worked out here (in the session scratchpad, one-line Python);
**[design]** a modelling choice.

## 1. Short answer

- **What players say they love is not a timbre but a relationship.** The recurring words are
  *feel* ("you feel it in your bones", Jimmy Smith), *body*, *sing like a human voice* (Booker T.
  Jones, about the Leslie and the swell), *a very strong physical relationship* and *it's not a
  volume pedal; it's an expression pedal* (Jon Lord), *an extension of me* (Dr. Lonnie Smith), *the
  most soulful instrument* (Cory Henry), and, from the church, that a pipe organ "couldn't shout"
  and a Hammond could (Bob Marovich) **[web]** (§2).
- **What players say clones miss** (forums and reviews, many voices, consistent) **[web]**:
  1. the key contacts: an on/off trigger and a click that sounds the same every time;
  2. each real organ's own amount of click and leakage;
  3. the chorus/vibrato ("close though");
  4. above all the Leslie: "clones are much closer to a real organ than a Leslie simulator is to a
     real tube Leslie"; "nothing really fills the space the way a real Hammond/tube Leslie does";
  5. overdrive that "will not follow the distortion of a vintage Hammond/Leslie setup when you
     change drawbars and play in different registers";
  6. the mechanical presence: "the feeling that you're sitting at the helm of a massive, powerful,
     potentially dangerous machine".

  A Sound On Sound history adds that samplers "can come very close when each note is played
  individually" **[web]**: so what fails is the **ensemble behaviour**, not the single note.
- **The causes, in one sentence [design, from §3]:** the B-3's tone itself is a static sum of
  near-sines; everything players respond to is **state and coupling** that their playing drives:
  - the momentum of two rotors;
  - the charge of the percussion capacitor;
  - the bias and supply of a tube amplifier that sees the whole organ at once;
  - nine contacts per key closing in a touch-dependent order onto wheels that never stop;
  - one shaft that locks every wheel's phase;
  - and the individual wear of one particular machine.

  A model built from free-running per-voice oscillators, an LFO-driven rotary effect and a
  post-mix volume pedal removes exactly these, and is what the complaints describe.
- **The ten properties to keep, ranked** (details §7):
  1. Leslie rotors as mechanics the player rides: continuous, asymmetric, horn ≠ drum, partial
     ramps, brake;
  2. the expression pedal driving one nonlinear chain (AO-28 + 122 amp) whose growl grows with the
     pedal, the number of keys and the drawbars;
  3. a key click from nine bouncing contacts, never identical twice, shaped by touch and
     registration;
  4. one phase-locked, always-running generator: shared wheels add coherently, no phase reset;
  5. Leslie acoustics: Doppler, directivity, reflections with their own Doppler, the 800 Hz split,
     distortion spun by the horn, and a room fed by a moving source;
  6. the percussion circuit: single trigger, partial recharge, the 1' steal, the level drop;
  7. drawbars and preset flips that move while notes sound, in the transformer's nine steps;
  8. one seed per "organ serial" for sidebands, leakage, capacitor age, contacts, Leslie speeds;
  9. the scanner chorus/vibrato as a delay line scanned by a capacitor rotor (C3);
  10. body and presence: the pedals' and the 15-inch woofer's low end, the noise floor, the
      mechanical noises, the motor.
- **Plans that must change (§6):** the MIDI CC numbers proposed for percussion collide with the
  clone conventions for the Leslie; velocity must also reach slow presses (harmonics entering one at
  a time) and per-key contact order; the Leslie needs an automatic "rider" for MIDI files without
  speed changes, a reachable brake, mechanical noises and a room fed by its moving source; CC11 must
  stay ahead of the Leslie amp; the drawbars need A/B preset "flips".

## 2. What players say

### 2.1 Jazz organists

- **Jimmy Smith** (DownBeat, 1994, interviewed by Dan Ouellette; read via Blue Note) **[web]**:
  "The Hammond has body. It's got depth—and resonance. It's got clarity—and quality. And you can
  feel it." "It's not so much that you can hear it. It's the feeling that's important." "It's like a
  drummer. You don't want to hear him. You want to feel him." "With the Hammond, you feel it in your
  bones."
- **Jimmy Smith** (JazzTimes, Sept. 1994, via Milkowski) **[web]**: "Then I cut my tremelo off to
  give it a big sound, and that sounded good." "I can put my tremelo to fast or medium or whatever
  to get the sound you want." So for him the Leslie's speed, including *off*, was a registration
  choice, and stopping it gave the "big" sound.
- **Dr. Lonnie Smith** **[web]**: the B-3 as "an extension of me" (NPR) **[web, snippet]**; the
  music as a spirit that "comes out through my fingers" (Albumism) **[web, snippet]**. On pedals,
  read directly: "Do you want a real legit sound? Do you want a grumpy sound? Do you want a warm
  sound?"; "I like for it to flow a lot" **[web, Milkowski IV]**. He moved his first B-3 himself:
  "It weighed over 400 pounds, but I was determined to move it".
- **Joey DeFrancesco** called the B-3 "a groove machine" and credited Jimmy Smith with showing what
  it could do **[web, snippet: SFJAZZ]**. **Counter-evidence:** he later judged the Diversi
  clone so good that he no longer needed a Leslie, and became a stakeholder **[web, snippet:
  Wikipedia "Diversi"]**; he called Hammond-Suzuki's digital B-3s hard to tell from originals
  **[web, snippet: Panken]**. A top player can accept a model, so the bar is reachable.
- **Barbara Dennerlein**: the B-3's sound "was her dream from childhood"; the pedals are "absolutely
  crucial" for her rhythm, "which the double bass can't easily imitate" **[web, snippet: AAJ,
  Kennedy Center]**. (She plays MIDI pedals with her own bass sound, so for her the *feet* matter
  more than the tonewheel pedal tone.)
- **Rhoda Scott** took her shoes off and played the pedals barefoot from childhood; she plays bass
  with her feet so that "a club owner didn't need to hire a bass player" **[web, snippet:
  Wikipedia, Myers 2011]**.
- **Larry Young, Jimmy McGriff:** no first-person statements about the sound were found. McGriff
  and Groove Holmes "spent a great deal of time experimenting and modifying their B3's and Leslie
  speakers", some of which Hammond and Leslie adopted **[web, snippet: Wikipedia]**: players
  treated the machine as something to voice.
- **Teaching material** (Bob Wijnen, *Hammond organ for the advanced jazz pianist*, 2022, read)
  **[web]**:
  - The expression pedal is used to "back off" after attacks, after ballad chords and in single
    lines; "you can pump the pedal back in to get more volume, highs and lows".
  - With percussion on, accents come from lifting the fingers, since "percussion only works when the
    notes are not connected".
  - "Squabbling": 800008888 with "the Leslie spins fast"; at a solo's climax "all drawbars out" and
    the Leslie to fast. Slides and swoops (Shirley Scott, Jimmy Smith; Mike LeDonne: "He might slide
    down the keyboard to the next chord").
- **Bass:** most jazz organists play bass with the left hand on the lower manual; the left foot taps
  a pedal for attack ("just hitting a pedal note slightly to obtain an attack effect") **[web,
  snippet: keyboardpartner forum; Helevä, Uniarts thesis]**.

### 2.2 Soul, rock and pop

- **Booker T. Jones** (Songfacts, read) **[web]**: "The first time I saw a Hammond organ I just got a
  feeling inside about it and I was comfortable." "It's a sustained sound as opposed to a piano …
  you can make it sustain louder or softer." "Because it has those Leslie cabinets and the horns
  turn, you can make it sing like a human voice." *Green Onions* was an M-3, "half of a B-3"; "I
  favor the sound of the M-3". "It's a very difficult instrument to maintain. It's not meant to be
  moved around." On digital organs: "I'm still not sure about it."
- **Jon Lord** (Keyboard, Jan. 1994, R. L. Doerschuk, read) **[web]**:
  - "A proper Hammond is a combination of electronics and mechanics." With the XB-2: "I'm still
    aware of the lack of a mechanical feel."
  - "There's a very strong physical relationship between me and the Hammond." "I'm used to standing
    behind a piece of furniture."
  - "It's not a volume pedal; it's an expression pedal."
  - Drive, not drawbars, gives the edge: "That's how I get the edge, rather than from using the top
    drawbars to cut through"; "this woody sort of sound, but with the amps and Leslies way up"; "one
    small Leslie, which we cranked up to get that nice growly sound".
  - Speed as structure: "I didn't want to spoil the build-up with a bunch of Leslie vibrato." Of a
    stationary Leslie: "It's less forgiving. Therefore it holds your attention just a little bit
    more."
  - And: "the Hammond organ, when treated like that, makes a heck of a noise" **[web, jonlord.org]**.
- **Keith Emerson:** "it's very difficult to make a 350lb Hammond part of you" (why he fought it on
  stage); knives in the keys gave "a great sustain"; turning it over made it feed back **[web,
  snippet: Counter Culture, Louder]**.
- **Steve Winwood** used Jimmy Smith's left-hand bass technique **[web, snippet: Keyboard]**;
  **Gregg Allman** ran a stock '69 B-3 into a 122RV **[web, snippet: MusicRadar]**; Warren Haynes
  called Allman's tone "impeccable", "the most important element of organ playing" **[web, snippet:
  Relix]**. **Billy Preston:** no first-person statement found; his rig is described as the B-3
  and its Leslie as "one of the essential elements" **[web, snippet: Mix]**.

### 2.3 The church

- **Bob Marovich** (NPR, 2016): the Hammond offered many sounds, and a pipe organ "couldn't shout"
  while a Hammond could **[web, snippet]**. The first Black church Hammond: First Church of
  Deliverance, Chicago, 1939 (Rev. Clarence Cobbs, music director Kenneth Morris) **[web,
  snippet]**.
- **Cory Henry** (Jazz Weekly, read): "I feel that the organ is the most soulful instrument that
  there is." He grew up with one at home and in church from age 4 **[web]**.
- **Ashon Crawley** (religious studies; a B-3 player in the Black Pentecostal church): the B-3 "is
  considered to constitute 'the sound' of the Black Church" **[web, UCHRI]**; of the chord changes,
  "They are actually playing what it feels like to be a question" **[web, Duke]**.
- **Braxton Shelley**, "Playing in Tongues: The Hammond Organ and Black Pentecostal
  Instrumentality", *Music & Science* 2025: Twinkie Clark's solos use a registration Hammond's
  manuals discourage, and "flips", where one timbral element gives way to another; each set of nine
  drawbars is "its own distinct timbral space" mapped onto spiritual and physical escalation
  **[paper, abstract via snippet]**. His talk title: "the Hammond Organ's Vocality" **[web]**.
- **Preaching chords:** the organist answers the preacher's "whooping" sermon close, call and
  response; the Hammond "can approximate what the voice does" **[web, snippet: NPR/WBUR,
  Wikipedia]**.

### 2.4 Technicians, history, forums

- **The click was a defect Hammond fought.** Hammond tried to eliminate or reduce it with filtering;
  jazz and rock players liked it and it became part of the sound **[web, snippet: Sweetwater;
  Wikipedia]**. Larry Heathcote: the click "is very difficult to filter out without serious
  degradation of the organ tone"; "the percussive attack relates to the note being keyed partway
  though its cycle"; Baldwin's resistive contact ramped the note in, and "In the Hammond there is no
  'roll'" **[web, SV-nature, read]**. Laurens Hammond also disliked the Leslie (and the "pop" it
  made audible) and kept it off his organs for decades **[web, snippet: Hammond Organ Co. history,
  PS Audio]**. So the two things players love most were both outside the maker's intent.
- **Touch matters, measurably.** Moro, McPherson and Sandler measured a Hammond's nine contacts:
  velocity, the type of touch and the key's position over time change the contacts' bounce, and in
  a listening study listeners could tell the type of touch from the sound **[paper, MMS17,
  abstract]**. Their follow-up: "Because of manufacturing tolerances, each contact closes at a
  different point in the key-throw … The Hammond is therefore touch-responsive, in such a way that
  cannot be captured with traditional velocity-based keyboard sensing." Players use "partial key
  presses (a key press that does not … trigger all of the key contacts), and very slow presses (…
  the harmonics start playing one at a time)" **[paper, MM20, read]**. Hammond-Suzuki's own XK-5
  page: "Depress a key slowly, however, and you hear the harmonics come in at slightly different
  times. Many beloved Hammond keyboard 'tricks' were born of this unintended imperfection" **[web,
  read]**.
- **Forums on clones** (organforum.com, Music Player Network) **[web, snippet]**:
  - "the big problem is the ON/OFF nature of the key contacts"; no clone has the key weight, strike
    and feel;
  - "Each HAMMOND has it's own unique level of keyclick and leakage"; an identical click on every
    key "sounds fake";
  - "one of the differences between real keyclick and many (all?) sims is that for the sims, it's
    the exact same sound each time"; the contact order "is not random, but it's not consistent,
    either";
  - "no clone has got the chor/vib right yet (close though)"; the same C/V settings sound different
    on different organs;
  - "clones are much closer to a real organ than a Leslie simulator is to a real tube Leslie";
    clones "sound best going through a real leslie";
  - "the one big element missing from the clones is the feeling that you're sitting at the helm of
    a massive, powerful, potentially dangerous machine"; a real B-3 "seems like it might overwhelm
    you and surround you at any moment" **[web, snippet: realgearonline]**;
  - a full clone rig with a real Leslie and 25 pedals gives "about 95% of the sound, feel, and
    control".
- **The Leslie ramp as a gesture** **[web, snippet: Music Player "Leslie speed control: foot switch
  or half moon, and why"]**: a player loves "the sound of the ramp up" and "weave[s] it around
  vocalists"; another habitually switches to fast "and then switch[es] off before it reaches full
  speed"; some use a momentary footswitch so that the Leslie sits on fast and goes to chorale only
  while held; some put the speed switch on the damper input so speed and expression go to different
  feet. Jazz players toggle between brake and fast **[web, snippet: Muzikus]**. Hammond's guide
  calls the inertia ramp "the most dramatic part of the sound" **[web, snippet]**. Mike Finnigan
  changes Leslie speeds "often and quickly" **[web, snippet: MusicRadar]**.
- **Overdrive:** the grit "comes from POWER amp tubes" of the Leslie, not the preamp **[web,
  snippet: Music Player "Jimmy Smith's overdrive on The Sermon"]**. Players on clones put the swell
  pedal on the virtual Leslie amp's gain because on a real Hammond "the whole point is to push the
  Leslie amp to breakup" **[web, snippet: Cantabile forum]**. A drive pedal review: it "will not
  follow the distortion of a vintage Hammond/Leslie setup when you change drawbars and play in
  different registers" **[web, snippet: Thomann]**. Henricksen: "When you really lean on it … a warm
  rich distortion like nothing you've heard" (`leslie.md` §7) **[paper]**.
- **Restorers:** Goff Professional recalibrates a generator "to the original factory calibration
  curve, or to another specific curve" designed for touring organists **[web, snippet]**: the
  wheels' level curve is a voicing choice. Wax-capacitor drift (2-4×) changes the top octaves
  (`hammond_b3.md` §3.4); recapping is debated, and one owner asks for same-organ before/after tests
  rather than videos **[web, snippet]**. A Mix article attributes the "organic quality" to small
  differences between points on the tone wheel **[web, snippet]**.
- **Percussion as a tool** **[web, snippet]**: "use the percussion's single-trigger to your
  advantage": legato notes get another timbre than staccato ones, tenuto in between; the decay can
  be shaped by how far the matching drawbar is pulled. Roland's VR-760 manual: on real tonewheel
  organs a very short gap between keys leaves the percussion under-charged, so it sounds quieter.
- **The motor:** turning RUN off and on gives a pitch dive of the whole organ as the generator
  coasts; holding START while running can also bend the pitch **[web, snippet: Wikipedia, manual
  excerpt]**; a forum post disputes how usable this is on a real B-3 **[web, snippet]**.
- **The other side.** Jim Alfredson: the B-3's status is "one of those fortuitous marraiges of
  luck, limited but ingeniously engineered technology, and hype"; "a C3 or an A100 … are just as
  good and sometimes even better. But the B3 has the name" **[web, organissimo, read]**. So the
  love attaches to the tonewheel-Leslie system, not to the B-3 cabinet as such: the model should
  treat B-3, C-3 and A-100 as one, and the M-3 as a close sibling (Booker T. prefers it).

## 3. Causes, and how sure we are

Confidence: **high** = many independent players and a physical mechanism; **medium** = several
voices or a primary document; **low** = one voice or an inference.

### 3.1 The sound

| Cause | Evidence | Confidence |
|---|---|---|
| **Additive sines and drawbars:** a near-pure, sustained, controllable spectrum; the drawbars as "timbral spaces" | Vorkoetter: "the crux of the Hammond sound" is additive vs subtractive; Shelley; Booker T. "sustain louder or softer" **[web; paper]** | high |
| **One shaft, locked phases, wheels shared between keys and drawbars** | Physics (`hammond_b3.md` §3.5, §7.2); samplers fine "when each note is played individually" but not in ensemble **[web]**; Electric Druid: the phase relationships' influence is "unknown" **[web]** | medium (mechanism certain; players do not name it) |
| **Imperfections: sidebands, leakage, aged capacitors** | "Each HAMMOND has it's own unique level of keyclick and leakage"; Mix "organic quality"; restorers' curves **[web]** | medium |
| **The key click** | Many forum voices; Moro et al. listening test; Hammond fought it, players kept it **[paper; web]** | high |
| **Percussion** | Jimmy Smith's registration (`hammond_b3.md` §11); "use the single trigger to your advantage" **[web]** | high for jazz, low for rock (often off) |
| **The scanner chorus/vibrato** | "no clone has got the chor/vib right yet"; C3 central to gospel and ballads **[web; memory]** | medium |
| **Overdrive** | Lord, Henricksen, the forums; "power amp tubes" **[web; paper]** | high for rock and gospel, medium for jazz |

- **The 7th harmonic is missing, and the "harmonics" are tempered** (5th 13-16 c sharp,
  `hammond_b3.md` §7.3) **[web, SV-nature; calc]**. Players do not name this, but it is why
  drawbar chords never fuse completely and why drive turns them into "growl" (§3.4) **[design]**.

### 3.2 The Leslie

| Cause | Evidence | Confidence |
|---|---|---|
| **The ramps, ridden as a gesture** | "love the sound of the ramp up", "weave it around vocalists", switching back before full speed, "most dramatic part" **[web]**; Lord's build-ups **[web]** | high |
| **The voice: Doppler + AM make the sustained tone "sing"** | Booker T. "sing like a human voice"; Shelley's "vocality"; Hammond "approximates what the voice does" **[web]** | high |
| **Room filling: a rotating, beaming source in a real room** | "nothing really fills the space"; Leslie sims judged worse than organ clones **[web]**; reflections each with their own Doppler **[paper, SSAB02]** | high (the complaint); medium (the mechanism) |
| **Horn ≠ drum** | the drum's slower response is "distracting" to some, who disconnect it **[paper, Henricksen]** | medium |
| **Stopped Leslie as a colour** | Jimmy Smith "cut my tremelo off to give it a big sound"; Lord "less forgiving" **[web]** | medium |

### 3.3 Touch and technique

| Cause | Evidence | Confidence |
|---|---|---|
| **No velocity; expression by the swell pedal** | Lord "not a volume pedal"; Wijnen "back off … pump the pedal back in … more volume, highs and lows" **[web]**; the compensation network (`hammond_b3.md` §10.3) **[paper]** | high |
| **Touch still matters through the contacts** | Moro et al.; XK-5 page; forum "ON/OFF" complaint **[paper; web]** | high |
| **Fast keys, waterfall action, glissandi, smears, slides** | Wijnen, LeDonne; "Ever do a smear on your clonewheel and sweep it off the stand?" (thread title) **[web]** | high |
| **Drawbar moves and preset flips while playing** | "all drawbars out" at climaxes; Shelley's "flips" **[web; paper]** | medium |
| **Percussion's single trigger as articulation** | forum, Wijnen, Roland **[web]** | high |
| **Bass: left hand plus pedal taps, or full pedals** | Lonnie Smith, Dennerlein, Rhoda Scott, Helevä **[web]** | medium (for the sound of the pedal attack) |

### 3.4 Interaction (the instrument answering the player)

- **Drive follows the player.** The 122 amp sees the sum of everything: both manuals, pedals,
  percussion. So its distortion rises with the pedal, with the number of keys and with the drawbars,
  and the bass notes intermodulate with the chords. That is what the drive-pedal review says is
  missing ("will not follow … when you change drawbars and play in different registers") **[web]**.
  Confidence **high** (mechanism certain, complaint explicit).
- **The rotor state carries over phrase boundaries.** A switch at a phrase end lands while the horn
  is still spinning down, and the drum lags by seconds (§4.1 numbers). The sound of the next phrase
  depends on what happened in the last one **[calc]**. **High.**
- **The percussion capacitor remembers.** A gap of 10 ms between legato notes re-arms only 25 % of
  the swing, 34 ms 63 %, 100 ms 95 %, so quasi-staccato playing gives softer, shorter pings (§4.6)
  **[calc from `hammond_b3.md` §8.2]**; Roland says the same **[web]**. **High.**
- **The amp breathes.** Cathode-bias shift and supply sag (`leslie.md` §7) compress after a loud
  chord and recover over 0.1-0.5 s **[memory; to check against the schematic]**. No player names it.
  **Low** as a cause, but it is part of "it responds".

### 3.5 Individuality

- Every organ has its own click, leakage, sideband pattern, capacitor age, generator calibration,
  contact wear, and its Leslie its own speeds, belt and amp **[web]** (`hammond_b3.md` §4-§5,
  `leslie.md` §4-§5). Owners talk about *their* organ; restorers sell a voicing. Confidence
  **medium**: well attested as a fact, less as a reason for love; but "sounds fake" is the reaction to
  its absence.

### 3.6 Presence and physicality

- Weight (300-425 lb plus the Leslie), furniture, standing "behind a piece of furniture" (Lord), the
  bones (Jimmy Smith), the "helm of a machine" **[web]**. A synthesizer cannot give weight; it can
  give what the body hears of it:
  - full low end (32-62 Hz pedal wheels through a 15-inch woofer);
  - the moving air of a rotating source in a room;
  - the mechanical sounds (motor, belt, horn wind, relay, keybed) and the hum and leakage floor.

  Confidence **medium** for the sound part, **high** that the physical part matters to players.
- **Warm-up:** no evidence found that players value a warm-up change in sound **[web, nothing
  found]**. Not a requirement.

### 3.7 Culture

- **Church:** the B-3 as "the sound" of the Black church, "shouting", answering the preacher,
  swelling with the congregation; gospel uses full drawbars, C3, frequent Leslie ramps
  (`hammond_b3.md` §11.2) **[web; memory]**. **High.**
- **Jazz trio:** Jimmy Smith's 888000000 with third-harmonic fast percussion, left-hand bass,
  "groove machine"; the Leslie mostly slow, fast for climaxes **[web; memory]**. **High.**
- **Rock:** drive and volume, "a heck of a noise", fast Leslie, physical abuse as theatre **[web]**.
  **High.**
- So one model must serve three playing cultures with different defaults. It does that through
  registrations, drive and how often the Leslie is switched (§5.5), not through three engines
  **[design]**.

## 4. Requirements

Each requirement has: **Mechanism** (the plan section that provides it, or *missing*),
**Destroyed by** (the typical digital mistake), **Test** (measurable). All **[design]** unless
marked; numbers are **[calc]** where derived.

### 4.1 R1: Leslie rotors the player can ride

- **Mechanism:** `leslie.md` §5.2: torque, belt slip, inertia and drag per rotor, integrated
  continuously; the speed switch only selects which motor is energised. Defaults horn τ ≈ 0.5 s up /
  0.8 s down, drum 3.5 s / 2.5 s (`leslie.md` §12.2).
- **Partial ramps fall out of it:** a fast switch held 0.7 s and then released takes the horn from
  0.80 to 5.2 Hz while the drum only reaches 1.6 Hz **[calc, first-order]**. That is the "switch off
  before it reaches full speed" gesture: a horn flutter over a still-slow drum.
- **To 90 % of fast:** horn 1.15 s, drum 8.1 s **[calc]**.
- **Stop/brake:** the rotors coast (or are DC-braked) to a stop at random angles. The resting angle
  then sets the static tone (horn facing the mic or away).
- **Destroyed by:**
  - an instant speed switch;
  - one LFO for both rotors;
  - a ramp that restarts from a fixed speed when the switch flips mid-ramp;
  - a phase reset at the switch (an audible jump);
  - identical horn and drum times;
  - a stop that always parks facing front.
- **Tests:**
  1. step slow→fast→slow: the speed is continuous (no step greater than the per-sample Δω of the
     motor torque), the horn reaches 63 % within τ ± 10 %, the drum is ≥ 4× slower;
  2. the 0.7 s tap above: horn peak 5.2 ± 0.5 Hz, drum ≤ 2 Hz;
  3. phase continuity: the instantaneous rotor angle has no discontinuity;
  4. five stops from fast: the parked horn angles are spread (circular standard deviation > 60°),
     and the HF level above 2 kHz differs by > 6 dB between the most front- and back-facing stops.

### 4.2 R2: One nonlinear chain that the pedal, the keys and the drawbars drive

- **Mechanism:**
  - CC11 drives the AO-28 expression capacitor with its compensation network (`hammond_b3.md`
    §10.1 item 7, §10.3), *before* V4b and the output stage, so the pedal sets level, tilt **and**
    the drive into the 122 amp (`leslie.md` §7);
  - the 122 amp: driver clip, push-pull 6550 saturation, cathode-bias shift, sag, output
    transformer;
  - the passive 800 Hz crossover after the amp, so a bass note's distortion products go to the
    rotating horn;
  - one amp for the whole organ (both manuals, pedals, percussion).
- **The amp sees a sum:** a single 888000000 note is 3 wheels; 2, 4 and 6 such notes raise the rms
  by 3.0, 6.0 and 7.8 dB if their wheels are distinct **[calc]**. For a soft clipper, 3rd-order
  products then rise ~2 dB per dB **[calc]**: a 4-note chord's growl is ~12 dB stronger relative
  to its tone than a single note's at the same pedal. Shared wheels add coherently (R4), so octave
  doublings drive harder than their rms suggests.
- **Destroyed by:**
  - CC11 as a post-mix volume (expression without drive);
  - a "drive" effect after a per-voice sum normalised by voice count;
  - a static waveshaper with no bias memory;
  - a separate distortion per voice;
  - an output limiter ahead of the Leslie that flattens the pedal;
  - distortion placed after the rotary simulation (it must be before the crossover, so it spins).
- **Tests:**
  1. at fixed pedal, THD+IMD (energy not at the input partials) rises monotonically with 1, 2, 4,
     6 held keys;
  2. at fixed keys, it rises monotonically with CC11;
  3. pulling 4' and 2' raises the distortion of a held chord (Lord's "edge" from amps, not from
     drawbars, works because the drawbars also feed the amp);
  4. a pedal C1 under an upper chord gives sidebands at chord partials ± 32.7 Hz that vanish when
     the amp is linearised;
  5. the distortion products above 800 Hz of a low chord show the horn's Doppler at the horn rate;
  6. after a 2 s ff chord, the gain of a following pp note recovers with a time constant in
     0.1-0.5 s;
  7. CC11 from 127 to 64 lowers the level by the network's dB **and** tilts the spectrum (more
     bass relative to treble, `hammond_b3.md` §10.3).

### 4.3 R3: A key click that is never the same twice and answers the touch

- **Mechanism:** `hammond_b3.md` §6.2: nine contacts per key, each a step onto a running wheel, with
  closure time, bounce and contact resistance. Additions **[design, from MM20, MMS17]**:
  - **per key and organ, a fixed nominal closing order and spacing of the nine contacts** (from
    the seed: manufacturing tolerance), plus per-press jitter, so "not random, but not consistent";
  - **velocity → key speed → stagger**: fast 0.1-0.5 ms, normal 1-3 ms. Below velocity ~20, a
    **slow press** of 10-100 ms (contacts spread over ~1-2 mm of travel at 10-100 mm/s **[calc,
    to check]**), so "the harmonics come in at slightly different times";
  - a **bounce model whose bounce count and duration depend on the contact's closing speed**
    (MM20's piecewise harmonic motion), with reduced amplitude excursion (they found the bounce AM
    smaller than full scale, through busbar capacitance and transformer inductance);
  - a release click from the same contacts, weaker;
  - dirty-contact noise from the age seed.
- **The click's spectrum then follows the registration with no free parameter:** 16'-only gives a
  low thump, the 1' a bright tick, and a step at a near-zero phase gives almost no click (rare).
- **Destroyed by:**
  - a click sample, or a filtered-noise burst, triggered per note;
  - one envelope for all nine footages;
  - a click whose level is a fixed knob independent of drawbars;
  - phase reset of the wheels at note-on (it fixes the step values, so every click is the same).
- **Tests:**
  1. 100 presses of one key at one velocity: the first 5 ms after onset, aligned, have pairwise
     correlation < 0.95 for ≥ 90 % of pairs, while after 20 ms the signals equal the generator's
     running sum (R4);
  2. the within-key mean correlation of clicks exceeds the between-key mean (the per-key order);
  3. click energy above 5 kHz with 1' pulled exceeds that with 16' only by > 15 dB;
  4. velocity 127 vs 20: the onset spread (time between first and last contact) differs by ≥ 5×;
     at velocity 5 the 9 footages are audibly staggered (≥ 10 ms first to last);
  5. a 20-key chromatic glissando at 30 ms per key gives 20 distinct onset transients.

### 4.4 R4: One phase-locked generator that never stops

- **Mechanism:** `hammond_b3.md` §13.1 item 1: 91 wheels from one master clock, integer phase
  accumulation, the shared speed factor, no phase reset. Keys and drawbars only gate (via contacts)
  wheels that are already running. Wheels shared between keys and between drawbars (foldback, §7.2)
  are one signal.
- **What a free-phase model does instead** **[calc]**: with 888000000, C4 and C5 share wheel 37
  (C4's 8' is C5's 16'). Locked, the two add coherently: +6 dB less loading, every time. With
  per-voice oscillators of random phase Δ, the sum is 20·log|2cos(Δ/2)|: anywhere from +6 dB to a
  hole, different on every press. The same happens in every octave doubling and every high-key
  foldback.
- **Destroyed by:**
  - per-voice oscillators;
  - phase reset at note-on;
  - per-note random detune;
  - "analogue drift" per voice;
  - polyphony limits and voice stealing (a smear holds 10-20 keys; the busbar model has no voices).
- **Tests:**
  1. render a chord held from t1 to t2 and the same chord held from t1′ to t2′: over their common
     interval, after 20 ms from the later onset, the outputs are sample-identical (output is a
     function of absolute time and the key set only);
  2. C4 + C5 at 888000000: the 130.8, 261.5 Hz … components' levels are the same on every press
     (spread < 0.1 dB over 50 presses at random times);
  3. 61 keys down at once: no note is dropped, CPU within budget (`hammond_b3.md` §13.4);
  4. the 2⅔' vs 3×8' drift (−0.62 Hz at C4) is present and identical between two renders started
     at the same absolute time.

### 4.5 R5: Leslie acoustics that sing and fill a room

- **Mechanism:** `leslie.md` §6, §12.1:
  - moving-tap delay lines per path and mic;
  - angle-dependent directivity per band;
  - early reflections as image sources, each with its own Doppler (the "chorus" of reflections,
    SSAB02);
  - the drum as AM above ~200 Hz, fixed below;
  - the cabinet tail;
  - two horn mics plus a drum mic.

  **Missing:** the room (`src/room.*`) is fed by the mid/side mix, so the room's reverb is a static
  reverb of a swirling signal. A real Leslie's beam *sweeps the room*, so the reverb itself
  fluctuates. Proposed: feed the room from 2-4 directional injection points (front, back, sides) with
  the horn's and drum's directivity gains for those directions, so the late field fluctuates at the
  rotor rates **[design]**.
- **Destroyed by:**
  - a stereo auto-pan or a chorus LFO;
  - FM and AM from one sine with no directivity;
  - one shared Doppler for all paths;
  - the drum treated like the horn (FM in the bass);
  - a fixed-impulse reverb after a static rotary;
  - mono collapse that cancels the FM.
- **Tests:** `leslie.md` §12.6 (Doppler depth, drum AM by band, stereo side signal, crossover).
  Added:
  1. with room on, the late reverberant energy (> 80 ms) in the 2-4 kHz band fluctuates at the horn
     rate by ≥ 1 dB (a static post-reverb gives ~0);
  2. mono sum (L+R) keeps a Doppler FM of ≥ 70 % of a single mic's.

### 4.6 R6: The percussion circuit, with memory

- **Mechanism:** `hammond_b3.md` §8, §13.1 item 6: C31's RC, the control-tube transfer with
  cut-off, single trigger on "any upper key", re-arm through R55 + R56 (τ = 34 ms), the 1' steal,
  the R1 level drop, soft/normal and fast/slow. **The re-arm must be partial.** The grid starts its
  next decay from where the recharge had reached when a key went down again **[calc]**:

| All-keys-up gap | Charge (of the 10 V swing) | Time to cut-off, slow / fast |
|---|---|---|
| 5 ms | 14 % | 0.14 / 0.03 s |
| 10 ms | 25 % | 0.24 / 0.06 s |
| 20 ms | 44 % | 0.40 / 0.10 s |
| 34 ms | 63 % | 0.55 / 0.13 s |
| 100 ms | 95 % | 0.76 / 0.18 s |
| full | 100 % | 0.79 / 0.19 s |

  The start level also falls with the charge (via the control tube's gain curve, to fit).
- **Destroyed by:**
  - a per-voice decaying partial (multi-trigger percussion);
  - an all-or-nothing re-arm;
  - percussion with vibrato (it joins after the scanner);
  - leaving the 1' sounding with percussion on;
  - no level drop.
- **Tests:** `hammond_b3.md` §13.6, plus:
  1. legato line: one ping on the first note only;
  2. the same line with 20 ms gaps: every note pings, each starting ≥ 6 dB softer and decaying to
     cut-off in 0.40 ± 0.06 s (slow) vs 0.79 s for a fully re-armed note;
  3. percussion on: the 1' drawbar has no effect on the upper manual.

### 4.7 R7: Drawbars and preset flips that move while notes sound

- **Mechanism:** `hammond_b3.md` §6.1, §7.1: each drawbar is a slider over nine transformer taps
  (Hamre's steps), on the busbar side, so moving it changes the level of every sounding key at
  once, in **steps**. Preset keys: the B-3 has two drawbar sets per manual (A# and B keys) plus fixed
  presets; switching is instant and is the gospel "flip" **[paper, HSM; paper, Shelley]**.
  **Missing:**
  - the sound of a drawbar passing between taps (a brief break or bridge; a soft tick)
    **[memory, to check on a real organ]**;
  - a MIDI control for the A/B flip (§5.3).
- **Destroyed by:**
  - drawbars read only at note-on;
  - continuous (unstepped) levels;
  - zipper noise from per-sample CC steps;
  - presets that retrigger notes.
- **Tests:**
  1. a CC sweep 0→127 over 1 s on a held note gives 8 level steps at Hamre's dB values (±0.5 dB),
     with each step's transition ≤ 5 ms and no other zipper;
  2. a preset flip on a held chord changes the spectrum within 5 ms with no onset click from the keys
     (only the preset-key contact's own small click).

### 4.8 R8: One seed per organ, and the restorer's voicing

- **Mechanism:** `hammond_b3.md` §4.6 ("organ serial number") draws eccentricities, tooth errors,
  capacitor ages, gains, leakage, tapering. Extend the one seed to:
  - the per-key contact order and spacing (R3), contact dirt;
  - the generator calibration curve (factory, or a "restorer's curve" as a named preset);
  - the Leslie: speeds ±3 %, belt age, the amp's tube balance (`leslie.md` §4, §5).
- **Destroyed by:**
  - a fixed "vintage noise" sample;
  - irregularity re-randomised every note or every run;
  - all wheels sharing one modulation pattern.
- **Tests:**
  1. the same seed renders bit-identically;
  2. two seeds: per-wheel sideband levels at f ± f_rot (long FFT, 0.1 Hz resolution) correlate
     < 0.5 across wheels, the leakage floor spectra differ by > 3 dB in at least a third of the
     bands, and the Leslie fast rates differ;
  3. the "age" knob at 0 brings any seed's sidebands below −50 dB.

### 4.9 R9: The scanner vibrato and chorus (C3 above all)

- **Mechanism:** `hammond_b3.md` §9, §13.1 item 7: the 18-section LC line (frequency-dependent
  delay and a ~7 kHz edge), a 9-tap capacitive crossfade scanned at 6.867 Hz from the generator
  motor (so mains-locked with the wheels), V1-V3, and C1-C3 as dry plus scanned.
- **Destroyed by:**
  - a sinusoidal LFO on a fractional delay (sine FM instead of the near-square FM of a triangular
    delay sweep);
  - a flat-delay line (no dispersion);
  - a scanner rate unrelated to the generator clock;
  - the chorus as a generic two-voice chorus.
- **Tests:**
  1. V3 on a 1 kHz wheel: FM at 6.867 Hz with deviation 18-26 c and an FM 3rd harmonic ≥ −15 dB
     relative to the fundamental (a sine LFO gives ≤ −40 dB);
  2. C3: moving comb notches whose depth differs between 500 Hz and 4 kHz (dispersion);
  3. the scanner rate scales exactly with the mains-drift factor applied to the wheels.

### 4.10 R10: Body, noise and machinery

- **Mechanism:**
  - pedals 1-12 as complex wheels through the pedal filter (`hammond_b3.md` §3.3, §3.4);
  - the 15-inch woofer's low end and the output-transformer saturation on pedal notes
    (`leslie.md` §7);
  - no high-pass in the chain above ~30 Hz;
  - the leakage background with no key down (`hammond_b3.md` §5), and tube hiss and hum (§10.3).

  **Missing in `leslie.md`:**
  - the motor hum;
  - the belt;
  - the horn's wind noise at fast speed (broadband, modulated at the horn rate);
  - the relay click on a speed change;
  - a soft keybed thump per key.

  **Optional:** the run-switch dive (generator coasting and re-locking) on pitch bend down
  (`hammond_b3.md` §13.3 already allows an "organ stop/start effect").
- **Pedal tapping:** a short pedal note under a left-hand bass line must give an attack (its own
  contacts' click on the complex wheels) without a long tone.
- **Destroyed by:**
  - a bass roll-off "for clarity";
  - a noise gate;
  - a digital-silence floor between notes;
  - pedal notes as plain sines with no click.
- **Tests:**
  1. a pedal C1 has energy at 32.7 Hz within 6 dB of its 65.4 Hz;
  2. with no key down and drawbars out, the output is the leakage-plus-hiss floor at its set level,
     not zero;
  3. at Leslie fast, the noise floor between notes is modulated at the horn rate;
  4. a 40 ms pedal note gives a transient whose energy above 1 kHz is ≥ 10 dB over its sustained
     part's.

## 5. Control mapping: keeping the gestures from MIDI

### 5.1 The expression pedal

- **CC11 → the expression capacitor's travel** (R2), with the taper of the compensation network,
  smoothed with a slew limit like a real pedal (~50-100 ms full travel) **[design]**. **CC7** is the
  output gain *after* the Leslie (a mixer fader), never the drive. The drive setting (the 122's
  volume, how hard the console drives the amp) is a per-program parameter: GM 17/18 clean-to-crunch
  at the top of the pedal, GM 19 driven (`hammond_b3.md` §13.3).
- **Velocity is not loudness** (`hammond_b3.md` §13.3; Lord, Booker T.). It is key speed (R3).

### 5.2 Live drawbars: the clone conventions

The conventions are incompatible, so the model needs a selectable map **[web, Apple EVB3 MIDI
modes, read; web, snippet: Nord forum]**:

| Convention | Upper drawbars | Lower drawbars | Leslie | Vibrato | Percussion |
|---|---|---|---|---|---|
| **RK** (Roland VK, Korg CX-3) | CC 70-78 | — | CC 80 or 92 chorale/brake/tremolo; 81 chorale/brake; 118 fast rate | 85 / 86 on, 87 type | 94 on, 95 2nd/3rd, 102 volume, 103 decay |
| **NE** (Nord Electro) | CC 16-24 | CC 70-78 | GP6 on (CC 81), GP7 speed (CC 82), GP8 brake (CC 83) | 85 / 86 on, 84 mode | 87 on, 88 volume/time, 95 harmonic |
| **NI** (Native Instruments B4) | CC 12-20 | CC 21-29 | GP7 fast (CC 82), GP8 accelerate (CC 83), mod wheel speed, CC 68 brake | 31 / 30 on | named only |
| **HS** (Hammond XB/XK) | CC 80, all upper (value-encoded) | CC 81 | named | 87 type | named |

- GP5-8 are CC 80-83 by the MIDI standard **[memory]**.
- **[design] Default: RK** for the drawbars and percussion (most hardware drawbar controllers send
  70-78), with NE and NI selectable.
- **Leslie, always on whatever the drawbar map:**
  - CC1 ≥ 64 fast, < 64 slow (`leslie.md` §9);
  - CC64 momentary or latching (Leslie 2101 default);
  - CC82 speed (NE/NI);
  - CC80/92 as a three-state half-moon (RK: low third chorale, middle brake, top third tremolo);
  - CC68 or CC83 brake.
- **The A/B preset flip (R7):** no convention found. Proposed: a toggle on a CC free in all four
  maps (e.g. CC 119; open question §8), or a program change within a bank of registrations
  **[design]**.

### 5.3 The half-moon and the brake

- **The half-moon** has chorale and tremolo, and often a stop in the middle **[web, snippet]**.
  The MIDI value map must allow a throw from tremolo to stop (brake) and from stop to chorale. Every
  transition goes through the rotor mechanics (R1). The "flick to fast, back before full speed"
  gesture needs nothing special: the mechanics give it.
- **The brake** stops the rotors, with the DC brake option (`leslie.md` §5.1), and parks at random
  angles (R1). `leslie.md` §9 proposes "CC1 = 0 held 1 s" for a stop. Players' files rarely do that
  (CC1 is a continuous wheel in the user's pop collection, `hammond_b3.md` §12.1), so the brake
  should also have its own CC (above) **[design]**.

### 5.4 What a MIDI file can still say

The user's files: one jazz file with no CC1, CC64 or CC11 changes; ~900 pop files of which only a
fraction send CC1 (a wheel) or CC64 (`hammond_b3.md` §12.1) **[file]**. Velocities are used as
dynamics. Without help most files would play an organ whose Leslie never changes speed and whose
pedal never moves: everything §2 says players love would be absent. So two automatic "hands", off
when the file supplies the control (any CC1/CC64/CC80/82/92 on the channel disables the Leslie
rider; any CC11 change disables the pedal rider) **[design]**.

### 5.5 The Leslie rider (rules)

Phrases are segmented as in the harmonica's hand rules (`harmonica.md` §5.2): onsets, rests,
held-note lengths. **[design; rules drawn from §2's players]**:

1. **Default:** chorale (GM 17, 18) or tremolo (GM 19).
2. **Up to fast:**
   - (a) on a phrase-climax note: a note or chord held ≥ 1 beat (or ≥ 0.6 s) that is the highest
     or loudest (velocity) of the last 2 bars;
   - (b) at the first onset after a rest ≥ 1 beat when the new phrase starts ≥ 5 semitones higher or
     ≥ 15 velocity louder than the last;
   - (c) when the left-hand density doubles (comping to shout).

   Switch on the beat **before** the target, so the horn arrives (1.1 s to 90 %) as the climax
   lands; the drum arrives later, which is right.
3. **Down to chorale:**
   - (a) at a rest ≥ 1 bar;
   - (b) on the last held chord of a section, 30-60 % into it (the spin-down *on* the final chord is
     the classic ending);
   - (c) never within 2 s of the last switch.
4. **Flick (partial ramp):** on a held chord ≥ 1.5 s in a slow passage, with probability p (jazz
   0.1, gospel 0.3, rock 0.15), switch to fast for 0.4-0.9 s and back: a horn swell over a slow drum
   (R1 test 2).
5. **Build-ups:** keep chorale through a crescendo (rising velocity over ≥ 2 bars) and release to
   fast at its peak (Lord).
6. **Stop:** jazz only: on a ballad's final chord, p = 0.2: brake ("cut my tremelo off").
7. **Rate limits:** fast dwell ≥ 2 s; ≤ 1 switch per 3 s; per-style averages: jazz ~1 change per
   16-32 bars, gospel 1 per 4-8 bars, rock as the music's sections.
8. **Determinism:** all random choices from the organ seed plus the note index, so a file renders
   the same every time.

### 5.6 The pedal rider (expression from velocity)

- **Target:** the pedal position = a slow function of the recent velocities, not per note:
  - the median velocity of the onsets in the last 0.5-1 s, mapped to CC11 over the upper ~15-20 dB
    of the pedal's range (most playing is near the top, Wijnen's "back off" is a dip);
  - slew-limited (rise ≥ 150 ms, fall ≥ 100 ms);
  - a dip of 3-6 dB 100-200 ms after accented chords ("back off after the attack"), then back.
- **The point:** the pedal then moves the drive as players do (R2), and per-note velocity never
  becomes per-note loudness.
- Off when the file sends CC11, and as an option (`hammond_b3.md` §14 question 4) **[design]**.

### 5.7 Velocity, and notes that MIDI cannot carry

- **Velocity → key speed:** the contact stagger and bounce (R3), including the slow-press regime at
  low velocities.
- **Partial key presses** (only some contacts close) cannot be expressed by note-on. Options:
  - polyphonic aftertouch at note-on time;
  - a "shallow" velocity band (1-8) that closes only the first k contacts of the key's order
    **[design; low priority]**.
- **Glissandi and smears** arrive as dense note runs; with no voice limit and per-key contacts they
  sound right by themselves (R3 test 5, R4 test 3).
- **Pitch bend:** ignored for pitch, optionally the motor coast (R10).

## 6. What must change in the existing plans

1. **`hammond_b3.md` §13.3, MIDI:**
   - "CC80-83 for percussion" collides with the Leslie in two conventions (RK rotor CC 80/81, NE and
     NI GP7/GP8 = CC 82/83). Move percussion to the RK numbers (94, 95, 102, 103);
   - make the drawbar CCs a selectable map (RK 70-78 default; NE 16-24 / 70-78; NI 12-20 / 21-29)
     instead of "CC12-20 or NRPN";
   - add the A/B preset flip (§5.2);
   - add the Leslie rider and the pedal rider (§5.5, §5.6) with their disable rules.
2. **`hammond_b3.md` §6.2 / §13.1 item 4, contacts:**
   - a per-key, per-organ fixed contact order and spacing from the seed, plus jitter;
   - velocity mapped down to a slow-press regime (10-100 ms stagger), not just 0.1-3 ms;
   - a bounce model driven by the contact's closing speed;
   - add R3's tests 1, 2, 4, 5 to §13.6.
3. **`hammond_b3.md` §8 / §13.6, percussion:** state that the re-arm is partial (the charge reached
   at the next key-down sets both the start level and the decay), and add the table of §4.6 as a
   test.
4. **`hammond_b3.md` §13.1 items 4 and 8:**
   - drawbars act on held notes, in steps;
   - CC11 stays inside the AO-28, ahead of the Leslie amp (already so in §10.3: make it a stated
     invariant);
   - CC7 is after the Leslie.
5. **`hammond_b3.md` §13.6:** add R4's absolute-time test and the shared-wheel coherence test (C4 +
   C5).
6. **`leslie.md` §9 / §12.3, control:**
   - add CC82 (NE/NI), the CC80/92 three-state half-moon and a brake CC, keeping CC1 and CC64;
   - the rider of §5.5 belongs to the organ instance (it reads the notes) and drives the Leslie's
     switch input.
7. **`leslie.md` §12.1, room:** feed `src/room.*` from directional injection points carrying the
   rotors' directivity, not only the mid/side mix (R5).
8. **`leslie.md` §12.1, noises:** add the motor hum, belt, horn wind noise (rate-modulated), relay
   click on speed changes.
9. **`leslie.md` §12.6, tests:** add the partial-ramp test, phase continuity and the brake's spread
   of stop angles (R1).
10. **Defaults:** keep `leslie.md`'s horn τ ≈ 0.5 s up. The players' "ramp up" love needs an
    *audible* horn ramp (about 1 s to 90 %), which this gives, and a drum several times slower.
    setBfree's 0.16 s horn is too fast to "ride" **[design]**.

## 7. The ten properties to keep, ranked

Ranking **[design]** by how often players name the property (§2), how certain the mechanism is, and
how completely a typical digital model destroys it.

| # | Property | Mechanism | Key test |
|---|---|---|---|
| 1 | **Leslie speed as a gesture** | rotor mechanics (`leslie.md` §5.2), horn ≠ drum, partial ramps, brake; the rider for files (§5.5) | 0.7 s tap: horn → 5.2 Hz, drum ≤ 2 Hz; continuous speed; spread stop angles (R1) |
| 2 | **The pedal drives one amp that hears everything** | CC11 in the AO-28 → 122 amp (bias, sag, transformer) → crossover → rotors; one amp for both manuals and pedals | distortion rises with pedal, key count and drawbars; pedal-note sidebands on chords; distortion spun by the horn (R2) |
| 3 | **A living key click** | 9 contacts per key, per-key order from the seed, velocity → stagger and bounce, steps onto running wheels | 100 presses never identical; brighter with 1'; slow press staggers footages (R3) |
| 4 | **One locked machine** | 91 wheels on one clock, no reset, shared wheels coherent, busbars with no voice limit | output depends only on absolute time and keys; C4 + C5 same level every press (R4) |
| 5 | **A Leslie that sings and fills the room** | moving taps per path and mic, directivity, image sources with own Doppler, drum AM, room fed by the moving source | late reverb fluctuates at the horn rate; mono keeps the FM (R5) |
| 6 | **Percussion with memory** | the C31 circuit, single trigger, partial re-arm, 1' steal, R1 drop | legato: one ping; 20 ms gaps: softer pings, 0.40 s decay (R6) |
| 7 | **Registration while playing** | drawbars in 9 taps on the busbars; A/B preset flips; CC maps | stepped sweep at Hamre's dB; flip within 5 ms with no key retrigger (R7) |
| 8 | **This organ, not an organ** | one seed: sidebands, leakage, capacitor age, contacts, calibration curve, Leslie speeds and belt | same seed bit-identical; two seeds differ in sidebands, floor, rates (R8) |
| 9 | **The scanner chorus/vibrato** | LC line with dispersion, 9-tap capacitive scan at 6.867 Hz, C3 = dry + scanned | near-square FM (3rd harmonic ≥ −15 dB); dispersive comb (R9) |
| 10 | **Body and machinery** | complex pedal wheels, full low end, leakage and hiss floor, motor, belt, horn wind, relay, keybed; optional motor coast | 32.7 Hz present; floor never digital silence; floor modulated at fast (R10) |

## 8. Open questions for the user

1. **The riders:** should the Leslie and pedal riders be on by default for files that lack the
   controls (§5.4), and with which style per GM program (jazz for 17/18, rock for 19, or gospel for
   17)?
2. **Drawbar CC map:** RK (70-78) as default, or the Nord convention, matching a controller you own?
3. **A/B flip CC:** which controller number (none of the conventions found has a standard one)?
4. **Slow-press regime:** may very low velocities (below ~20) mean "slow press" with harmonics
   entering one at a time, or should low velocities stay ordinary?
5. **Machinery noises** (motor, belt, horn wind, relay): on by default at a realistic level, or
   behind a knob at 0?
6. **A recording** remains the single most useful input (`hammond_b3.md` §14 question 1,
   `leslie.md` §13 question 1). For this paper's tests a few seconds of the same key pressed ten
   times, a speed change on a held chord, and a chord swelled with the pedal would settle R1-R3.

## 9. Sources

Read directly unless marked *snippet* (known only from a search engine's summary, the page being
blocked or timing out).

- **[DB94]** D. Ouellette, Jimmy Smith interview (DownBeat, 1994), quoted in "The Incredible Jimmy
  Smith: Back at the Chicken Shack", Blue Note.
  https://www.bluenote.com/spotlight/jimmy-smith-back-at-the-chicken-shack/
- **[Milk-III]** B. Milkowski, "In Praise of the Hulking 300 lb Beast, Part III" (Jimmy Smith,
  JazzTimes interview, Sept. 1994). https://www.billmilkowski.com/post/in-praise-of-the-hulking-300-lb-beast-part-iii
- **[Milk-IV]** B. Milkowski, "… Part IV" (Dr. Lonnie Smith on the pedals and his first B-3).
  https://www.billmilkowski.com/post/in-praise-of-the-hulking-300-lb-beast-part-iv ; Part I:
  https://www.billmilkowski.com/post/in-praise-of-the-hulking-300-lb-beast ; Part VI (Rhoda Scott,
  snippet): https://www.billmilkowski.com/post/in-praise-of-the-hulking-300-lb-beast-part-vi
- **[NPR-LS]** Dr. Lonnie Smith, NPR transcript (snippet). https://www.npr.org/transcripts/243984118 ;
  **[Alb-LS]** Albumism interview (snippet). https://albumism.com/interviews/jazz-legend-dr-lonnie-smith-feels-the-music-moves-the-crowd
- **[SFJ]** SFJAZZ, "'The Sleeping Bear' Awakens: The Revival of the Hammond B-3 Organ" (snippet;
  DeFrancesco "groove machine"). https://cdn.sfjazz.org/onthecorner/hammond-b3-organ-revival/
- **[Diversi]** Wikipedia, "Diversi" (snippet). https://en.wikipedia.org/wiki/Diversi ;
  **[Straight]** "Joey DeFrancesco is more than just a Jimmy Smith clone" (snippet).
  https://www.straight.com/article-178514/joey-defrancesco-more-just-jimmy-smith-clone ;
  **[Panken]** T. Panken blog (snippet). https://tedpanken.wordpress.com/tag/joey-defrancesco/ ;
  **[NAMM-JD]** NAMM Oral History, Joey DeFrancesco. https://www.namm.org/library/oral-history/joey-defrancesco
- **[AAJ-BD]** A. Bryson, "Barbara Dennerlein: A Study in Contrasts", All About Jazz (snippet).
  https://www.allaboutjazz.com/barbara-dennerlein-a-study-in-contrasts-barbara-dennerlein-by-alan-bryson ;
  **[KC-BD]** Kennedy Center bio (snippet). https://www.kennedy-center.org/artists/d/da-dn/barbara-dennerlein/
- **[RS]** Wikipedia, "Rhoda Scott" (snippet). https://en.wikipedia.org/wiki/Rhoda_Scott ; M. Myers
  interview, 2011 (snippet). https://jazzofilo.blogspot.com/2011/10/interview-rhoda-scott.html
- **[SF-BTJ]** Songfacts, Booker T. Jones interview. https://www.songfacts.com/blog/interviews/booker-t-jones
- **[KB94]** R. L. Doerschuk, "Jurassic Rock", Keyboard, Jan. 1994 (Jon Lord), on The Highway Star.
  https://thehighwaystar.com/interviews/lord/jl199401xx_2.html ; **[JL08]** "When a Hammond organ
  met hard rock". https://jonlord.org/2008/03/26/interview-when-a-hammond-organ-met-hard-rock/
- **[CC-KE]** Counter Culture, Keith Emerson interview (snippet).
  https://www.counterculture.co.uk/interview/the-nice-keith-emerson.html ; **[Louder-KE]** "The
  Prog interview: Keith Emerson" (snippet). https://www.loudersound.com/features/the-prog-interview-keith-emerson
- **[KB-SW]** Organissimo thread on the Keyboard interview with Steve Winwood (snippet).
  https://www.organissimo.org/forum/topic/31004-interview-with-steve-winwood-in-keyboard-magazine
- **[MR-GA]** MusicRadar, Gregg Allman on *Low Country Blues* (snippet).
  https://musicradar.com/news/guitars/gregg-allman-on-recording-low-country-blues-with-t-bone-burnett-363338 ;
  **[Relix]** Haynes on Allman (snippet). https://relix.com/?p=54133
- **[Mix-BP]** Mix, Robert Margouleff on recording Billy Preston (snippet).
  https://www.mixonline.com/recording/music-production/exclusive-excerpt-robert-margouleff-on-recording-billy-preston-part-1
- **[JW-CH]** Jazz Weekly, "Cory Henry: Having Church" (2025). https://jazzweekly.com/2025/06/cory-henry-interview
- **[NPR16]** A. Ford, "How The Hammond Organ Sound Laid The Tracks For Gospel's Hit Train", NPR,
  2016 (snippet; Marovich, Cobbs). https://www.npr.org/2016/01/03/461818544/how-the-hammond-organ-sound-laid-the-tracks-for-gospels-hit-train
- **[UCHRI]** A. Crawley, "To Be Made an Instrument of Feeling: The Hammond B-3 Organ in the Black
  Church". https://uchri.org/awards/to-be-made-an-instrument-of-feeling-the-hammond-b-3-organ-in-the-black-church/ ;
  **[Duke]** "Where did the Hammond sound come from and where did it go?"
  https://sites.duke.edu/dcore/2019/10/09/where-did-the-hammond-sound-come-from-and-where-did-it-go/
- **[Shelley25]** B. D. Shelley, "Playing in Tongues: The Hammond Organ and Black Pentecostal
  Instrumentality", *Music & Science* (2025), doi:10.1177/20592043241302531 (abstract via snippet;
  publisher page 403). https://journals.sagepub.com/doi/10.1177/20592043241302531 ; Yale CV (talk
  title). https://divinity.yale.edu/media/221/download
- **[Preach]** Wikipedia, "Preaching chords" (snippet). https://en.wikipedia.org/wiki/Preaching_chords
- **[Wijnen]** B. Wijnen, *Hammond organ for the advanced jazz pianist*, Research Catalogue (2022).
  https://researchcatalogue.net/view/1850935/1851034
- **[Helevä]** M. Helevä, *Bassolinjat Hammond B3 -uruilla*, Uniarts Helsinki (snippet).
  https://taju.uniarts.fi/bitstream/handle/10024/6344/mikko_helev__bassolinjat_hammond_b3_uruilla__jazzurkurin_nkkulma__est_35.pdf ;
  **[KP-tap]** keyboardpartner forum, "What is pedal tapping?" (snippet).
  https://forum.keyboardpartner.de/viewtopic.php?p=4710
- **[SOS-hist]** R. Spark, "The History Of The Hammond", Sound On Sound.
  https://www.soundonsound.com/people/history-hammond
- **[SV-nature]** S. Vorkoetter, "On the Nature of 'The Hammond Organ Sound'", with L. Heathcote's
  comment. https://www.stefanv.com/electronics/on-the-nature-of-the-hammond-organ-sound.html
- **[AAJ-Lat]** D. Latorre, "All About Jazz Organ: Inside the Hammond B3".
  https://www.allaboutjazz.com/all-about-jazz-organ-inside-the-hammond-b3-jimmy-smith
- **[Org-pin]** Organissimo, "Why is the B3 the pinnacle of organs?" (J. Alfredson).
  https://www.organissimo.org/forum/topic/34295-why-is-the-b3-the-pinnacle-of-organs-what-came-after/
- **[MMS17]** G. Moro, A. P. McPherson, M. B. Sandler, "Dynamic temporal behaviour of the keyboard
  action on the Hammond organ and its perceptual significance", J. Acoust. Soc. Am. 142(5):2808-2822
  (2017), doi:10.1121/1.5003796 (abstract via snippet). https://qmro.qmul.ac.uk/xmlui/handle/123456789/28574
- **[MM20]** G. Moro, A. P. McPherson, "A platform for low-latency continuous keyboard sensing and
  sound generation", NIME 2020 (read: §3, the Hammond emulator, contact order and bounce model).
  https://www.nime.org/proceedings/2020/nime2020_paper19.pdf ; thesis **[Moro19]** "Beyond key
  velocity" (snippet). https://qmro.qmul.ac.uk/xmlui/handle/123456789/67082
- **[XK5]** Hammond-Suzuki, XK-5 product page (multi-contact keyboard). https://hammondorganco.com/xk-5
- **[SW-click]** Sweetwater InSync, "Key Click" (snippet). https://www.sweetwater.com/insync/key-click/
- **[OF]** The Organ Forum threads (snippets): "Are we splitting hairs? Real tone wheels vs Clones",
  https://organforum.com/forums/forum/electronic-organs-midi/hammond-organs/11791-are-we-splitting-hairs-real-tone-wheels-vs-clones ;
  "Viscount vs. other Hammonds Clones",
  https://organforum.com/forums/forum/electronic-organs-midi/hammond-organs/666246-viscount-vs-other-hammonds-clones
- **[MP]** Music Player Network threads (snippets): "Are Hammond clones that different from the real
  thing?", https://forums.musicplayer.com/topic/154905-are-hammond-clones-that-different-from-the-real-thing/ ;
  "9 individual contacts per key, any clones do this?",
  https://forums.musicplayer.com/topic/126953-9-individual-contacts-per-key-any-clones-do-this ;
  "Leslie speed control: foot switch or half moon, and why",
  https://forums.musicplayer.com/topic/173796-leslie-speed-control-foot-switch-or-half-moon-and-why/page/3/ ;
  "My first time on organ" (percussion single trigger),
  https://forums.musicplayer.com/topic/153537-my-first-time-on-organ/page/7/ ;
  "Jimmy Smith's overdrive on The Sermon",
  https://forums.musicplayer.com/topic/169473-jimmy-smith%C2%92s-overdrive-on-%C2%93the-sermon%C2%93/ ;
  "Organ clones & Leslie sims: who's leading the digital race",
  https://forums.musicplayer.com/topic/177808-organ-clones-amp-leslie-sims-whos-leading-the-digital-race/
- **[RG]** realgearonline, "B3 emulations" (snippet). https://realgearonline.com/thread/1366/b3-emulations?page=2
- **[Cant]** Cantabile community, "Corky's VST organ tips" (snippet; swell on the Leslie amp gain).
  https://community.cantabilesoftware.com/t/corky-s-vst-organ-tips-and-tweaks-page/3330?page=45
- **[Thom]** Thomann, Tube Amp Manufactur Organ Drive, user review (snippet).
  https://www.thomann.co.uk/tube_amp_manufactur_organ_drive.htm
- **[Muz]** Muzikus, "Rock Organ XIX" workshop (snippet; brake/fast toggling).
  https://casopismuzikus.cz/workshopy/rock-organ-xix-workshop
- **[MR-MF]** MusicRadar, "Mike Finnigan's favourite organ settings" (snippet).
  https://www.musicradar.com/how-to/mike-finnigans-favourite-organ-settings-and-how-to-recreate-them
- **[VR760]** Roland VR-760 manual, percussion notes (snippet).
  https://music.manualsonline.com/manuals/mfg/roland/vr760.html?p=40
- **[Goff]** GOFF Professional, restoration and modification service (snippet).
  https://goffprof.com/products/goff-professional-restoration-and-modification-service
- **[Mix]** Mix magazine on the tone wheel's "organic quality" (snippet). https://www.mixonline.com/?p=83697
- **[HOC]** Hammond Organ Company, "The Hammond Story" (snippet). https://hammondorganco.com/the-hammond-story ;
  **[PSA]** PS Audio Copper, "Don Leslie's whirling dervish speaker contraption" (snippet).
  https://www.psaudio.com/copper/article/don-leslies-whirling-dervish-speaker-contraption/
- **[Run]** Wikipedia, "Hammond organ" (start/run switches, pitch dive; snippet).
  https://en.wikipedia.org/wiki/Hammond_organ
- **[EVB3-MIDI]** Apple, Logic/MainStage EVB3 "MIDI modes" (RK, HS, NI, NE CC maps).
  https://help.apple.com/mainstage/mac/2.2/en/mainstage/logicproinstruments/chapter_7_section_13.html ;
  **[Nord-CC]** Nord user forum (CC 82 rotor speed; snippet). https://www.norduserforum.com/viewtopic.php?p=168518
- **[Henricksen]**, **[SSAB02]**, **[HSM]**, **[L2101]**: as in `leslie.md` §14 and `hammond_b3.md`
  §15.
