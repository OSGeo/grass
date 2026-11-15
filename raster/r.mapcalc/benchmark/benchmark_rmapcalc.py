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
    mapsizes = [10e6, 50e6, 100e6]

    # run benchmarks

    for mapsize in mapsizes:
        benchmark(
            size=int(mapsize**0.5),
            step=0,
            results=results,
        )

    # plot results

    for metric in metrics:
        bm.nprocs_plot(
            results,
            filename=f"r_mapcalc_{metric}.svg",
            title=f"r.mapcalc {metric}",
            metric=metric,
        )


def benchmark(size, step, results):
    map1 = "benchmark_r_mapcalc_map1"
    map2 = "benchmark_r_mapcalc_map2"
    output = "benchmark_r_mapcalc"

    generate_map(rows=size, cols=size, fname=map1)
    generate_map(rows=size, cols=size, fname=map2)
    module = Module(
        "r.mapcalc",
        expression=f"{output}=({map1} + {map2})",
        overwrite=True,
    )

    results.append(
        bm.benchmark_nprocs(
            module,
            label=f"r.mapcalc_simple_{int((size * size) / 1e6)}M",
            max_nprocs=12,
            repeat=5,
        )
    )

    module = Module(
        "r.mapcalc",
        expression=f"{output}=({map1} + {map2} - 2*{map2} + {map1}*{map2} - {map1}/{map2})/2",
        overwrite=True,
    )
    results.append(
        bm.benchmark_nprocs(
            module,
            label=f"r.mapcalc_complex_{int((size * size) / 1e6)}M",
            max_nprocs=12,
            repeat=5,
        )
    )

    module = Module(
        "r.mapcalc",
        expression=f"{output}= if (({map1}[5, 5] + {map2}[-5, -5]) > 0, 1.6, 0.1)",
        overwrite=True,
    )
    results.append(
        bm.benchmark_nprocs(
            module,
            label=f"r.mapcalc_neighbor_{int((size * size) / 1e6)}M",
            max_nprocs=12,
            repeat=5,
        )
    )
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
