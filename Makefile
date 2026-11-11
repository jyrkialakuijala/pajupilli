CXX ?= g++
# Speed: AVX-512 with full 512-bit vectors (gcc otherwise often prefers 256-bit),
# fast-math for vectorised reductions; NaN/Inf checks kept for the output guard.
CXXFLAGS ?= -O3 -march=native -mprefer-vector-width=512 -ffast-math -fno-finite-math-only \
            -std=c++17 -Wall -Wextra
# Portable reference build (no AVX-512), to check the vector build against.
SCALARFLAGS = -O2 -std=c++17 -Wall -Wextra
LDLIBS = -lasound -lm

SRCS = $(wildcard src/*.cc)
HDRS = $(wildcard src/*.h)

flute: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(LDLIBS)

scalar: $(SRCS) $(HDRS)
	$(CXX) $(SCALARFLAGS) -o flute-scalar $(SRCS) $(LDLIBS)

# Show that the bore loops use zmm registers.
asm: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -S -o - src/bore.cc | grep -c zmm

clean:
	rm -f flute flute-scalar

.PHONY: scalar asm clean
