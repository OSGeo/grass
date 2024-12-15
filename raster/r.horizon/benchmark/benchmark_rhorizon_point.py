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
    mapsize = 1e7
    num_points = [10, 1e2, 1e3]

    # run benchmarks
    for n in num_points:
        benchmark(
            size=int(mapsize**0.5),
            npoint=int(n),
            step=0.5,
            label=f"r.horizon {int(n)} points",
            results=results,
        )

    # plot results
    for metric in metrics:
        bm.nprocs_plot(
            results,
            title=f"r.horizon point mode {metric}",
            metric=metric,
        )


def benchmark(size, npoint, step, label, results):
    reference = "benchmark_r_horizon_reference_map"
    output = "benchmark_r_horizon"

    w = 629993
    s = 214990
    rows = 3162
    cols = 3162
    coordinates = [(w + i % rows, s + i // cols) for i in range(npoint)]

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.horizon",
        elevation=reference,
        coordinates=coordinates,
        output=output,
        direction=0,
        step=step,
        quiet=True,
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
