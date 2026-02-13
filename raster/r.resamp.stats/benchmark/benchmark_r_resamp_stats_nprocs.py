"""Benchmarking of r.resamp.stats

@author Vinay Chopra
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    # Users can add more or modify existing reference maps
    benchmark(7071, "r.resamp.stats_50M", results)
    benchmark(10000, "r.resamp.stats_100M", results)
    benchmark(14142, "r.resamp.stats_200M", results)

    bm.nprocs_plot(results, filename="r_resamp_stats_benchmark_nprocs.svg")


def benchmark(size, label, results):
    reference = "benchmark_r_resamp_stats_input"
    output = "benchmark_r_resamp_stats_output"

    generate_map(rows=size, cols=size, fname=reference)

    # Set coarser output region for resampling (20x coarser)
    Module("g.region", flags="p", rows=size // 20, cols=size // 20)

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
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=16, repeat=3))
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
