CXX ?= g++
# Speed: AVX-512 with full 512-bit vectors (gcc otherwise often prefers 256-bit),
# fast-math for vectorised reductions; NaN/Inf checks kept for the output guard.
CXXFLAGS ?= -O3 -march=native -mprefer-vector-width=512 -ffast-math -fno-finite-math-only \
            -std=c++17 -Wall -Wextra
# Portable reference build (no AVX-512), to check the vector build against.
SCALARFLAGS = -O2 -std=c++17 -Wall -Wextra
LDLIBS = -lasound -lm -pthread

SRCS = $(wildcard src/*.cc)
HDRS = $(wildcard src/*.h)

pajupilli: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(LDLIBS)

scalar: $(SRCS) $(HDRS)
	$(CXX) $(SCALARFLAGS) -o pajupilli-scalar $(SRCS) $(LDLIBS)

# Show that the bore loops use zmm registers.
asm: $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) -S -o - src/bore.cc | grep -c zmm

clean:
	rm -f pajupilli pajupilli-scalar

# Fast checks (PLAN.md step 1): vectorisation, stability, NaN recovery,
# tuning and registers, scalar parity, real-time margin. Some groups, in parallel:
# make test T='jazz_guitar fx_*' J=4 (tools/test.py -l lists the groups).
test: pajupilli scalar
	python3 -I tools/test.py $(if $(J),-j $(J)) $(T)

.PHONY: scalar asm clean test
