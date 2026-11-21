#include "heard.h"

#include <cstdarg>
#include <mutex>
#include <queue>
#include <vector>

#include "songclock.h"

namespace pajupilli {
namespace {

std::mutex mu;
bool live = false;
struct Item {
  double t;
  long order;  // (said in this order at one moment: kept so)
  std::string text;
  std::FILE* f;
};
struct Later {
  bool operator()(const Item& a, const Item& b) const { return a.t != b.t ? a.t > b.t : a.order > b.order; }
};
std::priority_queue<Item, std::vector<Item>, Later> due;  // (the earliest on top)
long said = 0;

void put(const Item& it) {
  std::string s = it.text;
  while (!s.empty() && s.back() == '\n') s.pop_back();
  std::fprintf(it.f, "%8.3f s  %s\n", it.t, s.c_str());
}

std::string vstrf(const char* fmt, va_list ap) {
  va_list ap2;
  va_copy(ap2, ap);
  char buf[512];
  const int n = std::vsnprintf(buf, sizeof buf, fmt, ap);
  std::string s;
  if (n < int(sizeof buf)) s = buf;
  else {
    s.resize(n + 1);
    std::vsnprintf(&s[0], n + 1, fmt, ap2);
    s.resize(n);
  }
  va_end(ap2);
  return s;
}

}  // namespace

std::string strf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  std::string s = vstrf(fmt, ap);
  va_end(ap);
  return s;
}

void say(std::FILE* f, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  const std::string s = vstrf(fmt, ap);
  va_end(ap);
  const SongClock& c = SongClock::get();
  if (!c.playing) {  // (not -play: as it always was)
    std::fputs(s.c_str(), f);
    return;
  }
  sayAt(c.now, s, f);
}

void sayAt(double t, const std::string& text, std::FILE* f) {
  std::lock_guard<std::mutex> lock(mu);
  const Item it{t, said++, text, f};
  if (live) {
    due.push(it);
    return;
  }
  put(it);
  std::fflush(f);
}

void heardLive(bool on) {
  std::lock_guard<std::mutex> lock(mu);
  live = on;
  if (on) return;
  while (!due.empty()) {
    put(due.top());
    due.pop();
  }
  std::fflush(stdout);
  std::fflush(stderr);
}

void heardUntil(double t) {
  std::lock_guard<std::mutex> lock(mu);
  bool any = false;
  while (!due.empty() && due.top().t <= t) {
    put(due.top());
    due.pop();
    any = true;
  }
  if (any) {
    std::fflush(stdout);
    std::fflush(stderr);
  }
}

}  // namespace pajupilli
