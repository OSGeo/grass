"""Benchmarking of r.mapcalc
raster (2D)

@author Chung-Yuan Liang, 2025
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module

import grass.benchmark as bm


def main():
    results = []
    metrics = ["time", "speedup", "efficiency"]
    mapsizes = [10e6, 20e6]

    # run benchmarks

    for mapsize in mapsizes:
        benchmark(
            size=int(mapsize**0.5),
            step=0,
            label=f"r.mapcalc{int(mapsize / 1e6)}M",
            results=results,
        )

    # plot results

    for metric in metrics:
        bm.nprocs_plot(
            results,
            title=f"r.mapcalc {metric}",
            metric=metric,
        )


def benchmark(size, step, label, results):
    map1 = "benchmark_r_mapcalc_map1"
    map2 = "benchmark_r_mapcalc_map2"
    output = "benchmark_r_mapcalc"

    generate_map(rows=size, cols=size, fname=map1)
    generate_map(rows=size, cols=size, fname=map2)
    module = Module(
        "r.mapcalc",
        expression=f"{output}=({map1} + {map2} - 2*{map2} + {map1}*{map2} - {map1}/{map2})/2",
        overwrite=True,
    )

    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=8, repeat=3))
    Module(
        "g.remove", quiet=True, flags="f", type="raster", pattern="benchmark_r_mapcalc*"
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
