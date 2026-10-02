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

The plots show the mean of the repeated runs with a shaded 95 % confidence
interval of the mean (Student's t). The interval of the speedup and of the
efficiency combines the relative intervals of the two times divided.

@author Vaclav Petras
"""

import math
import statistics
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

# Two-sided 95 % quantiles of Student's t by the number of runs (degrees of
# freedom plus one), to avoid depending on SciPy.
T_QUANTILES = {2: 12.706, 3: 4.303, 4: 3.182, 5: 2.776, 6: 2.571, 7: 2.447}


def main():
    if len(sys.argv) > 1:
        results = bm.load_results_from_file(sys.argv[1]).results
    else:
        results = []
        for resolution in RESOLUTIONS:
            benchmark(resolution, results)
        bm.save_results_to_file(results, "r_sim_water_benchmark.json")
    plot(results)


def mean_and_half_width(times):
    """Return the mean and the half width of its 95 % confidence interval"""
    mean = statistics.mean(times)
    if len(times) < 2:
        return mean, 0
    t = T_QUANTILES.get(len(times), 1.96)
    return mean, t * statistics.stdev(times) / math.sqrt(len(times))


def plot(results):
    """Plot time, speedup and efficiency with confidence bands"""
    import matplotlib as mpl  # pylint: disable=import-outside-toplevel

    mpl.use("Agg")
    import matplotlib.pyplot as plt  # pylint: disable=import-outside-toplevel

    for metric, ylabel in [
        ("time", "Time [s]"),
        ("speedup", "Speedup"),
        ("efficiency", "Efficiency"),
    ]:
        # Twice the 600 pixels of the documentation, shown at half the size.
        fig, ax = plt.subplots(figsize=(6, 4.5), dpi=200)
        for result in results:
            means, halves = zip(
                *(mean_and_half_width(t) for t in result.all_times), strict=True
            )
            serial, serial_half = means[0], halves[0]
            values, lows, highs = [], [], []
            for nprocs, mean, half in zip(result.nprocs, means, halves, strict=True):
                if metric == "time":
                    value, relative = mean, half / mean
                else:
                    value = serial / mean
                    # The speedup at one thread is 1 by definition.
                    relative = (
                        math.hypot(serial_half / serial, half / mean)
                        if nprocs > 1
                        else 0
                    )
                    if metric == "efficiency":
                        value /= nprocs
                values.append(value)
                lows.append(value * (1 - relative))
                highs.append(value * (1 + relative))
            (line,) = ax.plot(result.nprocs, values, label=result.label)
            ax.fill_between(
                result.nprocs,
                lows,
                highs,
                color=line.get_color(),
                alpha=0.25,
                linewidth=0,
            )
        ax.set_xticks(results[0].nprocs)
        ax.set_xlabel("Number of threads (nprocs)")
        ax.set_ylabel(ylabel)
        ax.set_title(f"r.sim.water {metric}")
        ax.legend()
        fig.tight_layout()
        fig.savefig(f"r_sim_water_benchmark_{metric}.png")
        plt.close(fig)


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
