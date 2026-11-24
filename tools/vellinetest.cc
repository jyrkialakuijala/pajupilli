#include "../src/velline.h"
#include <cstdio>
#include <cstdlib>
using namespace pajupilli;
int main(int argc, char** argv) {  // args: vel:beat:len ... (len in beats; tpq 480)
  std::vector<VelNote> p;
  for (int i = 1; i < argc; ++i) {
    double v, b, l;
    if (std::sscanf(argv[i], "%lf:%lf:%lf", &v, &b, &l) != 3) return 1;
    p.push_back({long(b * 480), long((b + l) * 480), int(v)});
  }
  for (const VelReading& r : readVelocities(p, 480)) std::printf("%.0f>%.0f%+.0f ", r.line, r.lineEnd, r.accent);
  std::printf("\n");
}
