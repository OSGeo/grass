"""Benchmarking of r.horizon
point mode, one direction

@author Chung-Yuan Liang, 2024
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module

import grass.benchmark as bm


def main():
    results = []
    metrics = ["time", "speedup", "efficiency"]
    mapsizes = [1e6, 2e6, 4e6, 8e6]

    # run benchmarks
    for mapsize in mapsizes:
        benchmark(
            size=int(mapsize**0.5),
            step=0,
            label=f"r.horizon_{int(mapsize / 1e6)}M",
            results=results,
        )

    # plot results
    for metric in metrics:
        bm.nprocs_plot(
            results,
            title=f"r.horizon raster mode {metric}",
            metric=metric,
        )


def benchmark(size, step, label, results):
    reference = "benchmark_r_horizon_reference_map"
    output = "benchmark_r_horizon"

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.horizon",
        elevation=reference,
        output=output,
        direction=0,
        step=step,
    )

    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=8, repeat=3))
    Module(
        "g.remove", quiet=True, flags="f", type="raster", pattern="benchmark_r_horizon*"
    )


def generate_map(rows, cols, fname):
    Module("g.region", flags="p", rows=rows, cols=cols, res=1)
    # Generate using r.random.surface if r.surf.fractal fails
    try:
        print("Generating reference map using r.surf.fractal...")
        Module("r.surf.fractal", output=fname, overwrite=True)
    except CalledModuleError:
        print("r.surf.fractal fails, using r.random.surface instead...")
        Module("r.random.surface", output=fname, overwrite=True)


if __name__ == "__main__":
    main()
