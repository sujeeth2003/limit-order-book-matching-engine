CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
LDFLAGS  ?= -pthread

all: build/test_book build/bench_all build/simd_bench

build:
	mkdir -p build

build/test_book: tests/test_book.cpp $(wildcard */*.hpp) | build
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

build/bench_all: bench/bench_all.cpp $(wildcard */*.hpp) | build
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

build/simd_bench: v4_avx2/simd_bench.cpp v4_avx2/book.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

test: build/test_book
	./build/test_book

bench: build/bench_all
	./build/bench_all 1000000 2 3000

simd: build/simd_bench
	./build/simd_bench

# Look for cmov vs. conditional jumps in the v3 hot path
asm:
	sh scripts/asm_check.sh

