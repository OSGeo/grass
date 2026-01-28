#!/bin/bash

g.region raster=elevation res=10

echo "Benchmark: r.watershed reuse feature"
echo ""
echo "Scenario: Generate RUSLE LS factor with different parameters"
echo "- WITHOUT reuse: Run full r.watershed each time"
echo "- WITH reuse: Generate flow maps once, then reuse"
echo ""

g.remove -f type=raster pattern="bench_*" 2>/dev/null

echo "Baseline (WITHOUT reuse - full calculation):"
hyperfine --warmup 1 --runs 3 \
  --prepare 'g.remove -f type=raster name=bench_ls 2>/dev/null' \
  'r.watershed elevation=elevation threshold=1000 length_slope=bench_ls max_slope_length=100'

echo ""
echo "Generate flow maps:"
r.watershed elevation=elevation accumulation=bench_accum drainage=bench_drain
echo "   Flow maps generated."

echo ""
echo "WITH reuse (using pre-generated flow maps):"
hyperfine --warmup 1 --runs 5 \
  --prepare 'g.remove -f type=raster name=bench_ls 2>/dev/null' \
  'r.watershed elevation=elevation accumulation_input=bench_accum drainage_input=bench_drain threshold=1000 length_slope=bench_ls max_slope_length=100'

echo ""
echo "Summary"
echo "The 'WITH reuse' benchmark shows the time for EACH iteration when"
echo "reusing the same flow maps. The one-time flow generation cost is"
echo "amortized across multiple runs with different parameters."

g.remove -f type=raster pattern="bench_*"
