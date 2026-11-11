// Every model idea is a knob: an environment variable with a default.
#pragma once

#include <cstdlib>

namespace pajupilli {

inline double knob(const char* name, double def) {
  const char* v = std::getenv(name);
  return v && *v ? std::atof(v) : def;
}

}  // namespace pajupilli
