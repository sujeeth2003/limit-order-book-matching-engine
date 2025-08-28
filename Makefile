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

