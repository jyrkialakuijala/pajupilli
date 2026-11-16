// Something that vibrates as modes and can be touched at a point: a drum
// head (membrane.h), a cymbal (plate.h). What touches it (a brush, a stick, a
// wire, another surface's rim) asks for each mode's share at its place once,
// and with it reads the height and speed there and pushes.
#pragma once

namespace pajupilli {

class Surface {
 public:
  virtual ~Surface() = default;
  virtual int modes() const = 0;
  virtual double radius() const = 0;
  // Each mode's share at (r from the centre, m; theta, rad) into phi[modes()].
  virtual void shape(double r, double theta, float* phi) const = 0;
  virtual const float* q() const = 0;  // the modes' displacements
  virtual const float* v() const = 0;  // ... and velocities
  virtual float* force() = 0;          // generalised forces for this sample
  double height(const float* phi) const {
    const float* x = q();
    float s = 0;
    for (int k = 0, n = modes(); k < n; ++k) s += phi[k] * x[k];
    return s;
  }
  double speed(const float* phi) const {
    const float* x = v();
    float s = 0;
    for (int k = 0, n = modes(); k < n; ++k) s += phi[k] * x[k];
    return s;
  }
  void push(const float* phi, float f) {  // a force (N, upward) there
    float* x = force();
    for (int k = 0, n = modes(); k < n; ++k) x[k] += phi[k] * f;
  }
};

}  // namespace pajupilli
