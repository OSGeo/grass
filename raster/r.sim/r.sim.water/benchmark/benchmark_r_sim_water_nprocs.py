"""Benchmarking of r.sim.water with different numbers of threads

Runs r.sim.water with nprocs from 1 to MAX_NPROCS on the elevation raster
of the SECREF North Carolina dataset (secref_northcarolina_usa_epsg6542)
at several resolutions of the same extent. At resolutions coarser than
the original 1 m, the elevation is the average of the 1 m cells. Otherwise
r.sim.water runs with its defaults, so the number of walkers (twice the
number of cells) and the number of iterations grow with finer resolution.

Run in a temporary mapset of the dataset, for example:

    grass --tmp-mapset ~/grassdata/secref_northcarolina_usa_epsg6542 \
        --exec python benchmark_r_sim_water_nprocs.py

The plots and the results as JSON are written to the current directory.
To redo the plots from saved results without running the benchmark, pass
the JSON file as an argument.

The plots show the mean of the repeated runs with the range of the runs
shaded (see grass.benchmark.nprocs_plot).

@author Vaclav Petras
"""

import sys
from subprocess import DEVNULL

from grass.pygrass.modules import Module

import grass.benchmark as bm
import grass.script as gs

# Users can modify the resolutions (in meters), the number of threads and
# the number of repeats. With the default duration, the serial run at 1 m
# takes about two minutes.
RESOLUTIONS = [4, 2, 1]
MAX_NPROCS = 16
REPEAT = 3


def main():
    if len(sys.argv) > 1:
        results = bm.load_results_from_file(sys.argv[1]).results
    else:
        results = []
        for resolution in RESOLUTIONS:
            benchmark(resolution, results)
        bm.save_results_to_file(results, "r_sim_water_benchmark.json")
    plot(results)


def plot(results):
    """Plot time, speedup and efficiency with the range of the runs"""
    for metric in ["time", "speedup", "efficiency"]:
        # Twice the 600 pixels of the documentation, shown at half the size.
        bm.nprocs_plot(
            results,
            filename=f"r_sim_water_benchmark_{metric}.png",
            title=f"r.sim.water {metric}",
            metric=metric,
            figsize=(6, 4.5),
            dpi=200,
        )


def benchmark(resolution, results):
    elevation = "benchmark_r_sim_water_elevation"
    dx = "benchmark_r_sim_water_dx"
    dy = "benchmark_r_sim_water_dy"
    depth = "benchmark_r_sim_water_depth"

    Module("g.region", raster="elevation")
    Module("g.region", res=resolution, flags="a")
    # Average the 1 m cells rather than taking the nearest one.
    Module(
        "r.resamp.stats",
        input="elevation",
        output=elevation,
        method="average",
        overwrite=True,
    )
    Module("r.slope.aspect", elevation=elevation, dx=dx, dy=dy, flags="e")
    cells = gs.region()["cells"]

    module = Module(
        "r.sim.water",
        elevation=elevation,
        dx=dx,
        dy=dy,
        depth=depth,
        random_seed=1,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
        quiet=True,
    )
    results.append(
        bm.benchmark_nprocs(
            module,
            label=f"{cells / 1000:.0f}k cells ({resolution} m)",
            max_nprocs=MAX_NPROCS,
            repeat=REPEAT,
        )
    )
    Module(
        "g.remove",
        quiet=True,
        flags="f",
        type="raster",
        pattern="benchmark_r_sim_water_*",
    )


if __name__ == "__main__":
    main()
