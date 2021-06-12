"""Benchmarking of r.univar

@author Aaron Saw Min Sern
"""

from grass.pygrass.modules import Module
from subprocess import DEVNULL
import os

ITERATION = 5
UP_TO_THREADS = 12


def benchmark(module):
    term_size = os.get_terminal_size()
    print(module.get_bash())

    min_ave = float("inf")
    min_t = 1
    for t in range(1, UP_TO_THREADS + 1):
        print("\u2500" * term_size.columns)
        print(f"Benchmark with {t} thread(s)...\n")
        sum = 0
        for _ in range(ITERATION):
            module(nprocs=t).run()
            print(f"{module.time}s")
            sum += module.time

        ave = sum / ITERATION
        if t == 1:
            serial_ave = ave
        if ave < min_ave:
            min_ave = ave
            min_t = t
        print(f"\nResult - {ave}s")

    print("\u2500" * term_size.columns)
    print(f"\nSerial average time - {serial_ave}s")
    print(f"Best average time - {min_ave}s ({min_t} threads)\n")


def main():
    multiple_maps_parallel = Module(
        "r.univar", map=["elevation"] * 10, flags="g", stdout_=DEVNULL
    )
    multiple_maps_with_zone_parallel = Module(
        "r.univar",
        map=["elevation"] * 10,
        zones="basin_50K",
        flags="g",
        stdout_=DEVNULL,
    )

    benchmark(multiple_maps_parallel)
    benchmark(multiple_maps_with_zone_parallel)


if __name__ == "__main__":
    main()
