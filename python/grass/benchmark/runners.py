# MODULE:    grass.benchmark
#
# AUTHOR(S): Aaron Saw Min Sern <aaronsms u nus edu>
#            Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Benchmarking for GRASS GIS modules
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.


"""Basic functions for benchmarking modules"""

import shutil
from types import SimpleNamespace

import grass.script as gs


def benchmark_nprocs(module, label, max_nprocs, repeat):
    """Benchmark module using values of nprocs up to *max_nprocs*.

    *module* is an instance of PyGRASS Module class.
    The module is executed  used to generate range of values from 1 up to *max_nprocs*.
    *repeat* sets how many times the each run is repeated.
    So, the module will run ``max_nprocs * repeat`` times.

    *label* is a text to add to the result (for user-facing display).
    Optional *nprocs* is passed to the module if present.

    Returns an object with attributes *times* (list of average execution times),
    *all_times* (list of lists of measured execution times), *nprocs*
    (list of *nprocs* values used), and *label* (the provided parameter as is).
    """
    term_size = shutil.get_terminal_size()
    print(module.get_bash())

    min_avg = float("inf")
    min_time = 1
    avg_times = []
    all_times = []
    nprocs_list = list(range(1, max_nprocs + 1))
    for nprocs in nprocs_list:
        print("\u2500" * term_size.columns)
        print(f"Benchmark with {nprocs} thread(s)...\n")
        time_sum = 0
        measured_times = []
        for _ in range(repeat):
            module(nprocs=nprocs).run()
            print(f"{module.time}s")
            time_sum += module.time
            measured_times.append(module.time)

        avg = time_sum / repeat
        avg_times.append(avg)
        all_times.append(measured_times)
        if nprocs == 1:
            serial_avg = avg
        if avg < min_avg:
            min_avg = avg
            min_time = nprocs
        print(f"\nResult - {avg}s")

    print("\u2500" * term_size.columns)
    print(f"\nSerial average time - {serial_avg}s")
    print(f"Best average time - {min_avg}s ({min_time} threads)\n")

    return SimpleNamespace(
        all_times=all_times,
        times=avg_times,
        nprocs=nprocs_list,
        label=label,
    )


def benchmark_resolutions(module, resolutions, label, repeat=5, nprocs=None):
    """Benchmark module using different resolutions.

    *module* is an instance of PyGRASS Module class.
    *resolutions* is a list of resolutions to set (current region is currently
    used and changed but that may change in the future).
    *repeat* sets how many times the each run is repeated.
    So, the module will run ``len(resolutions) * repeat`` times.

    *label* is a text to add to the result (for user-facing display).
    Optional *nprocs* is passed to the module if present.

    Returns an object with attributes *times* (list of average execution times),
    *all_times* (list of lists of measured execution times), *resolutions*
    (the provided parameter as is), *cells* (number of cells in the region),
    and *label* (the provided parameter as is).
    """
    term_size = shutil.get_terminal_size()
    print(module.get_bash())

    avg_times = []
    all_times = []
    n_cells = []
    for resolution in resolutions:
        gs.run_command("g.region", res=resolution)
        region = gs.region()
        n_cells.append(region["cells"])
        print("\u2500" * term_size.columns)
        print(f"Benchmark with {resolution} resolution...\n")
        time_sum = 0
        measured_times = []
        for _ in range(repeat):
            if nprocs:
                module(nprocs=nprocs)
            module.run()
            print(f"{module.time}s")
            time_sum += module.time
            measured_times.append(module.time)

        avg = time_sum / repeat
        avg_times.append(avg)
        all_times.append(measured_times)
        print(f"\nResult - {avg}s")

    return SimpleNamespace(
        all_times=all_times,
        times=avg_times,
        resolutions=resolutions,
        cells=n_cells,
        label=label,
    )
