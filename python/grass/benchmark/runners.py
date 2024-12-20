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

import random
import shutil
from types import SimpleNamespace

import grass.script as gs


def benchmark_single(module, label, repeat=5):
    """Benchmark module as is without changing anything.

    *module* is an instance of PyGRASS Module class or any object which
    has a *run* method which takes no arguments and executes the benchmarked code,
    and attribute *time* which is set to execution time after the *run*
    function returned. Additionally, the object should be convertible to *str*
    for printing.

    *repeat* sets how many times the each run is repeated.
    *label* is a text to add to the result (for user-facing display).

    Returns an object with attributes *time* (an average execution time),
    *all_times* (list of measured execution times),
    and *label* (the provided parameter as is).
    """
    term_size = shutil.get_terminal_size()
    if hasattr(module, "get_bash"):
        print(module.get_bash())
    else:
        print(module)

    min_avg = float("inf")

    print("\u2500" * term_size.columns)
    time_sum = 0
    measured_times = []
    for _ in range(repeat):
        module.run()
        print(f"{module.time}s")
        time_sum += module.time
        measured_times.append(module.time)

    avg = time_sum / repeat
    min_avg = min(avg, min_avg)
    print(f"\nResult - {avg}s")

    print("\u2500" * term_size.columns)
    print(f"Best average time - {min_avg}s\n")

    return SimpleNamespace(
        all_times=measured_times,
        time=avg,
        label=label,
    )


def benchmark_nprocs(module, label, max_nprocs, repeat=5, shuffle=True):
    """Benchmark module using values of nprocs up to *max_nprocs*.

    *module* is an instance of PyGRASS Module class or any object which
    has a *update* method taking *nprocs* as a keyword argument,
    a *run* which takes no arguments and executes the benchmarked code,
    and attribute *time* which is set to execution time after the *run*
    function returned. Additionally, the object should be convertible to *str*
    for printing.

    The module is executed for each generated value of nprocs. *max_nprocs* is used
    to generate a continuous range of integer values from 1 up to *max_nprocs*.
    *repeat* sets how many times the each run is repeated.
    So, the module will run ``max_nprocs * repeat`` times.
    Runs are executed in random order, set *shuffle* to false if they
    need to be executed in order based on number of threads.

    *label* is a text to add to the result (for user-facing display).
    Optional *nprocs* is passed to the module if present.

    Returns an object with attributes *times* (list of average execution times),
    *all_times* (list of lists of measured execution times),
    *efficiency* (parallel efficiency), *nprocs* (list of *nprocs* values used),
    and *label* (the provided parameter as is).
    """
    term_size = shutil.get_terminal_size()
    if hasattr(module, "get_bash"):
        print(module.get_bash())
    else:
        print(module)

    min_avg = float("inf")
    min_time = None
    serial_avg = None
    result = SimpleNamespace(times=[], all_times=[], speedup=[], efficiency=[])
    result.nprocs = list(range(1, max_nprocs + 1))
    result.label = label
    nprocs_list_shuffled = sorted(result.nprocs * repeat)
    if shuffle:
        random.shuffle(nprocs_list_shuffled)
    times = {}
    print("\u2500" * term_size.columns)
    for nprocs in nprocs_list_shuffled:
        module.update(nprocs=nprocs)
        module.run()
        print(f"Run with {nprocs} thread(s) took {module.time}s\n")
        if nprocs in times:
            times[nprocs] += [module.time]
        else:
            times[nprocs] = [module.time]
    for nprocs in sorted(times):
        avg = sum(times[nprocs]) / repeat
        result.times.append(avg)
        result.all_times.append(times[nprocs])
        if nprocs == 1:
            serial_avg = avg
        if avg < min_avg:
            min_avg = avg
            min_time = nprocs
        result.speedup.append(serial_avg / avg)
        result.efficiency.append(serial_avg / (nprocs * avg))

    print("\u2500" * term_size.columns)
    if serial_avg is not None:
        print(f"\nSerial average time - {serial_avg}s")
    print(f"Best average time - {min_avg}s ({min_time} threads)\n")

    return result


def benchmark_resolutions(module, resolutions, label, repeat=5, nprocs=None):
    """Benchmark module using different resolutions.

    *module* is an instance of PyGRASS Module class or any object
    with attributes as specified in :func:`benchmark_nprocs`
    except that the *update* method is required only when *nprocs* is set.

    *resolutions* is a list of resolutions to set (current region is currently
    used and changed but that may change in the future).
    *repeat* sets how many times the each run is repeated.
    So, the module will run ``len(resolutions) * repeat`` times.

    *label* is a text to add to the result (for user-facing display).
    Optional *nprocs* is passed to the module if present
    (the called module does not have to support nprocs parameter).

    Returns an object with attributes *times* (list of average execution times),
    *all_times* (list of lists of measured execution times), *resolutions*
    (the provided parameter as is), *cells* (number of cells in the region),
    and *label* (the provided parameter as is).
    """
    term_size = shutil.get_terminal_size()
    if hasattr(module, "get_bash"):
        print(module.get_bash())
    else:
        print(module)

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
                module.update(nprocs=nprocs)
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
