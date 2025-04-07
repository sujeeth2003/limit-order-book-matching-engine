#!/bin/sh
# Compare v2 (branchy) and v3 (select-based) machine code: count conditional
# jumps vs cmov across the whole translation unit. Fewer jcc + more cmov in v3
# is the goal, but only the benchmark and `perf stat -e branch-misses` decide
# whether it actually helped on your CPU. Run from the repo root.
CXX=${CXX:-g++}
mkdir -p build
for v in v2_flat_array v3_branchless; do
  ns=${v%%_*}
  printf '#include "../%s/book.hpp"\nvoid use(%s::Book& b) { b.add(1, Buy, 5, 1); (void)b.cancel(1); }\n' "$v" "$ns" > build/asm_$v.cpp
  $CXX -std=c++20 -O2 -S -o build/asm_$v.s build/asm_$v.cpp || { echo "compile failed for $v"; continue; }
  printf '%-16s jcc=%s  cmov=%s\n' "$v" \
    "$(grep -cE '^[[:space:]]+j(e|ne|l|le|g|ge|a|ae|b|be|s|ns|z|nz)[[:space:]]' build/asm_$v.s)" \
    "$(grep -cE '^[[:space:]]*cmov' build/asm_$v.s)"
done
