"""Benchmarking of r.resamp.stats

Benchmarks r.resamp.stats with a 25M cell input (5000x5000) at multiple
coarsening ratios (5x, 10x, 15x, 30x) to evaluate parallel efficiency
under different resolution scenarios.

@author Vinay Chopra
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    # Benchmark at different coarsening ratios with 25M cell input (5000x5000)
    # Using 5000 instead of 10000 to keep runtime manageable on laptops.
    for ratio in [5, 10, 15, 30]:
        benchmark(5000, ratio, f"r.resamp.stats_25M_{ratio}x", results)

    try:
        bm.nprocs_plot(results, filename="r_resamp_stats_benchmark_nprocs.svg")
        print("\nPlot saved to: r_resamp_stats_benchmark_nprocs.svg")
    except ImportError:
        print(
            "\nmatplotlib not available — text summary only.\n"
            "Install with: pip install matplotlib"
        )

    # Always print text summary regardless of matplotlib
    print("\n--- Benchmark Summary ---")
    for r in results:
        serial_time = r.times[0]
        best_time = min(r.times)
        best_nprocs = r.nprocs[r.times.index(best_time)]
        speedup = serial_time / best_time
        print(
            f"{r.label}: serial={serial_time:.2f}s, "
            f"best={best_time:.2f}s ({best_nprocs} threads), "
            f"speedup={speedup:.2f}x"
        )


def benchmark(size, ratio, label, results):
    reference = "benchmark_r_resamp_stats_input"
    output = "benchmark_r_resamp_stats_output"

    generate_map(rows=size, cols=size, fname=reference)

    # Set coarser output region for resampling
    Module("g.region", flags="p", rows=size // ratio, cols=size // ratio)

    module = Module(
        "r.resamp.stats",
        input=reference,
        output=output,
        method="average",
        flags="w",
        memory=300,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=10, repeat=3))
    Module(
        "g.remove",
        quiet=True,
        flags="f",
        type="raster",
        name=(reference, output),
    )


def generate_map(rows, cols, fname):
    Module("g.region", flags="p", rows=rows, cols=cols, res=1)
    # Generate using r.surf.fractal if available, fall back to r.random.surface
    try:
        print("Generating reference map using r.surf.fractal...")
        Module("r.surf.fractal", output=fname, overwrite=True)
    except CalledModuleError:
        print("r.surf.fractal fails, using r.random.surface instead...")
        Module("r.random.surface", output=fname, overwrite=True)


if __name__ == "__main__":
    main()
