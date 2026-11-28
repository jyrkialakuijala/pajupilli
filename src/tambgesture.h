// One gesture of the tambourine player (src/tambplan.h, research/tambourine.md 1.2):
// what makes the jingles move, how it is held, how strong, what follows.
#pragma once

namespace pajupilli {

struct TambGesture {
  enum Drive {
    kAgainst,    // struck against something: the other hand's heel, the thigh, the knee (the frame stops)
    kAir,        // a turn in the air: the frame reverses (dir: +1 down, -1 up)
    kTwist,      // a quick rotation of the wrist: the pairs thrown round the ring
    kHead,       // a stroke on the head (striker, place)
    kShakeRoll,  // a shake roll of dur s, from speed to speed2
    kThumbRoll,  // a thumb (finger) roll along the head of dur s
    kChoke,      // the jingles stopped: flat, the hand on the head
  };
  enum Striker { kFingertips, kFingers, kFist, kPalm, kHeel, kThigh, kKnee };
  Drive drive = kHead;
  Striker striker = kFingers;
  int fingers = 2;        // (kFingertips: one to three)
  float place = 0.5f;     // where on the head: 0 the rim (over the jingles) .. 1 the centre
  float height = 0.5f;    // where it is held: 0 low, against the thigh .. 0.5 the chest .. 1 high in the air
  float tilt = 0.5f;      // 0 flat .. 0.5 at 45 degrees .. 1 upright
  float speed = 1.0f;     // the hand's (frame's) speed at the stroke, m/s
  float speed2 = 1.0f;    // (a roll: at its end)
  float lift = 0.0f;      // the small jingle as the motion begins, 0 .. 1 of the stroke
  float lead = 0.0f;      // s from now until the stroke lands (the lift fills it)
  float dur = 0.0f;       // a roll's length (s); 0 for a held key: until released
  int dir = 1;            // (kAir) +1 a down stroke, -1 an up stroke
  enum Follow { kRing, kHold, kStop } follow = kRing;
  float stopAfter = 0.12f;  // (kStop: s after the stroke)
};

}  // namespace pajupilli
