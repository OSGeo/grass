# MODULE:    grass.benchmark
#
# AUTHOR(S): Aaron Saw Min Sern
#            Vaclav Petras
#
# PURPOSE:   Benchmarking for GRASS GIS modules
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.


"""Basic functions for benchmarking modules"""

import os
from types import SimpleNamespace

import grass.script as gs


def benchmark_nprocs(module, label, max_threads, repeat):
    term_size = os.get_terminal_size()
    print(module.get_bash())

    min_avg = float("inf")
    min_time = 1
    avg_times = []
    all_times = []
    for nprocs in range(1, max_threads + 1):
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
        avg_times=avg_times,
        threads=list(range(1, max_threads + 1)),
        label=label,
    )


def benchmark_resolutions(module, resolutions, label, repeat, nprocs):
    term_size = os.get_terminal_size()
    print(module.get_bash())

    avg_times = []
    all_times = []
    for resolution in resolutions:
        gs.run_command("g.region", res=resolution)
        print("\u2500" * term_size.columns)
        print(f"Benchmark with {resolution} resolution...\n")
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
        print(f"\nResult - {avg}s")

    return SimpleNamespace(
        all_times=all_times, avg_times=avg_times, resolutions=resolutions, label=label
    )
