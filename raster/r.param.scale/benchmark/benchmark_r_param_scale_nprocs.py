"""Benchmarking of r.param.scale
raster (2D)
"""

# import os

from grass.exceptions import CalledModuleError, GrassError
from grass.pygrass.modules import Module
import grass.benchmark as bm

# Baselines held fixed while one dimension is swept.
BASE_MAPSIZE = 50e6  # cells
BASE_WINDOW = 9  # window side, must be odd
BASE_MEMORY = 300  # MB
MAPSIZES = [10e6, 50e6, 100e6]
WINDOWS = [3, 9, 19, 39]
MEMORIES = [10, 100, 300, 1000]
METRICS = ["time", "speedup", "efficiency"]


def main():
    # Pin one thread per physical core so threads do not share a core's
    # hyperthreads. The OpenMP runtime reads these when it initializes in each
    # r.param.scale subprocess, so they must be set before the first run; the
    # subprocesses inherit them from this environment.
    # os.environ["OMP_PROC_BIND"] = "close"
    # os.environ["OMP_PLACES"] = "cores"

    # Sweep raster size at the baseline window and memory.
    results = []
    for mapsize in MAPSIZES:
        benchmark(
            size=int(mapsize**0.5),
            window=BASE_WINDOW,
            memory=BASE_MEMORY,
            label=f"r.param.scale_{int(mapsize / 1e6)}M",
            results=results,
        )
    plot(results, "rastersize")

    # Sweep window size at the baseline raster size and memory.
    results = []
    for window in WINDOWS:
        benchmark(
            size=int(BASE_MAPSIZE**0.5),
            window=window,
            memory=BASE_MEMORY,
            label=f"r.param.scale_window_{window}",
            results=results,
        )
    plot(results, "window")

    # Sweep memory at the baseline raster size and window.
    results = []
    for memory in MEMORIES:
        benchmark(
            size=int(BASE_MAPSIZE**0.5),
            window=BASE_WINDOW,
            memory=memory,
            label=f"r.param.scale_memory_{memory}MB",
            results=results,
        )
    plot(results, "memory")


def benchmark(size, window, memory, label, results):
    reference = "benchmark_r_param_scale_reference"
    output = "benchmark_r_param_scale"
    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.param.scale",
        input=reference,
        output=output,
        method="feature",
        size=window,
        memory=memory,
        run_=False,
        overwrite=True,
    )
    results.append(
        bm.benchmark_nprocs(
            module,
            label=label,
            max_nprocs=12,
            repeat=3,
        )
    )
    Module(
        "g.remove",
        quiet=True,
        flags="f",
        type="raster",
        pattern="benchmark_r_param_scale*",
    )


def plot(results, sweep):
    for metric in METRICS:
        bm.nprocs_plot(
            results,
            filename=f"r_param_scale_{sweep}_{metric}.svg",
            title=f"r.param.scale {sweep} {metric}",
            metric=metric,
        )


def generate_map(rows, cols, fname):
    Module("g.region", flags="p", n=rows, e=cols, res=1, w=0, s=0)
    # Generate using r.random.surface if r.surf.fractal fails
    try:
        print("Generating reference map using r.surf.fractal...")
        Module("r.surf.fractal", output=fname, overwrite=True)
    except (CalledModuleError, GrassError):
        print("r.surf.fractal fails, using r.random.surface instead...")
        Module("r.random.surface", output=fname, overwrite=True)


if __name__ == "__main__":
    main()
