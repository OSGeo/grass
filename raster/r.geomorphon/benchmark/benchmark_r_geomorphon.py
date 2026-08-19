"""Benchmarking of r.geomorphon
raster (2D)
"""

from grass.exceptions import CalledModuleError, GrassError
from grass.pygrass.modules import Module
import grass.benchmark as bm

# Baselines held fixed while one dimension is swept.
BASE_MAPSIZE = 50e6  # cells
BASE_SEARCH = 25  # outer search radius in cells
BASE_MEMORY = 300  # MB
MAPSIZES = [10e6, 50e6, 100e6]
SEARCHES = [10, 25, 50]
METRICS = ["time", "speedup", "efficiency"]


def main():
    # Sweep raster size at the baseline search radius.
    results = []
    for mapsize in MAPSIZES:
        benchmark(
            size=int(mapsize**0.5),
            search=BASE_SEARCH,
            memory=BASE_MEMORY,
            label=f"r.geomorphon_{int(mapsize / 1e6)}M",
            results=results,
        )
    plot(results, "rastersize")

    # Sweep search radius at the baseline raster size.
    results = []
    for search in SEARCHES:
        benchmark(
            size=int(BASE_MAPSIZE**0.5),
            search=search,
            memory=BASE_MEMORY,
            label=f"r.geomorphon_search_{search}",
            results=results,
        )
    plot(results, "search")


def benchmark(size, search, memory, label, results):
    reference = "benchmark_r_geomorphon_reference"
    output = "benchmark_r_geomorphon"
    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.geomorphon",
        elevation=reference,
        forms=output,
        search=search,
        memory=memory,
        run_=False,
        overwrite=True,
    )
    results.append(
        bm.benchmark_nprocs(
            module,
            label=label,
            max_nprocs=8,
            repeat=3,
        )
    )
    Module(
        "g.remove",
        quiet=True,
        flags="f",
        type="raster",
        pattern="benchmark_r_geomorphon*",
    )


def plot(results, sweep):
    for metric in METRICS:
        bm.nprocs_plot(
            results,
            filename=f"r_geomorphon_{sweep}_{metric}.svg",
            title=f"r.geomorphon {sweep} {metric}",
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
