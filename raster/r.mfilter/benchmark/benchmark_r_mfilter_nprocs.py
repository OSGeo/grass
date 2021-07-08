"""Benchmarking of r.mfilter

@author Aaron Saw Min Sern
"""

from grass.pygrass.modules import Module
from grass.script import tempfile
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    benchmark(7071, "r.mfilter_50M", results)
    benchmark(10000, "r.mfilter_100M", results)
    benchmark(14142, "r.mfilter_200M", results)
    benchmark(20000, "r.mfilter_400M", results)

    bm.nprocs_plot(results)


def benchmark(length, label, results):
    fractal = "benchmark_fractal"
    output = "benchmark_r_mfilter_nprocs"
    filter = tempfile()
    with open(filter, "w") as w:
        w.write(
            """MATRIX 9
                   1 1 1 1 1 1 1 1 1
                   1 2 1 2 1 2 1 2 1
                   1 1 3 1 3 1 3 1 1
                   1 2 1 4 1 4 1 2 1
                   1 1 3 1 5 1 3 1 1
                   1 2 1 4 1 4 1 2 1
                   1 1 3 1 3 1 3 1 1
                   1 2 1 2 1 2 1 2 1
                   1 1 1 1 1 1 1 1 1
                   DIVISOR 81
                   TYPE    P"""
        )

    generate_fractal(rows=length, cols=length, fname=fractal)
    module = Module(
        "r.mfilter",
        input=fractal,
        output=output,
        filter=filter,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=16, repeat=3))
    Module("g.remove", quiet=True, flags="f", type="raster", name=fractal)
    Module("g.remove", quiet=True, flags="f", type="raster", name=output)


def generate_fractal(rows, cols, fname):
    Module("g.region", flags="p", s=0, n=rows, w=0, e=cols, res=1)
    Module("r.surf.fractal", output=fname)


if __name__ == "__main__":
    main()
