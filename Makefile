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

# puhuja (src/puhuja: a copy of the speech synthesizer's sources, the lead singer's voice; every
# change there is recorded with its motivation in src/puhuja/CHANGES.md). Built apart and only
# when it changes (its largest file is a megabyte), with puhuja's own flags: no -ffast-math,
# which lets glibc swap exp/log/pow in vectorized loops.
PUHUJA_SRCS = $(wildcard src/puhuja/*.cc)
PUHUJA_HDRS = $(wildcard src/puhuja/*.h src/puhuja/*.inc)
PUHUJA_OBJS = $(patsubst src/puhuja/%.cc,build/puhuja/%.o,$(PUHUJA_SRCS))
PUHUJA_FLAGS ?= -O3 -march=native -fno-math-errno -fno-signed-zeros -fno-trapping-math -ffp-contract=off -std=c++17
PUHUJA_LIB = build/libpuhuja.a
.DEFAULT_GOAL := pajupilli

build/puhuja/%.o: src/puhuja/%.cc $(PUHUJA_HDRS)
	@mkdir -p build/puhuja
	$(CXX) $(PUHUJA_FLAGS) -c -o $@ $<

$(PUHUJA_LIB): $(PUHUJA_OBJS)
	ar rcs $@ $(PUHUJA_OBJS)

pajupilli: $(SRCS) $(HDRS) $(PUHUJA_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS) $(PUHUJA_LIB) $(LDLIBS)

scalar: $(SRCS) $(HDRS) $(PUHUJA_LIB)
	$(CXX) $(SCALARFLAGS) -o pajupilli-scalar $(SRCS) $(PUHUJA_LIB) $(LDLIBS)

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

# The players practise their slurs and trills again (after a change to an instrument or its
# player): searches the gestures, rewrites src/slur_data.h, rebuilds, checks. Long (an hour or
# more for all). make slurs I='oboe' for one instrument (tools/slurs.py; research/slurs.md).
slurs: pajupilli
	python3 -I tools/slurs.py recalibrate $(I)

.PHONY: scalar asm clean test slurs
