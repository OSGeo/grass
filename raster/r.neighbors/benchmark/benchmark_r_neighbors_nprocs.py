"""Benchmarking of r.neighbors

@author Aaron Saw Min Sern
"""

from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    benchmark(7071, "r.neighbors_50M", results)
    benchmark(10000, "r.neighbors_100M", results)
    benchmark(14142, "r.neighbors_200M", results)
    benchmark(20000, "r.neighbors_400M", results)

    bm.nprocs_plot(results)


def benchmark(length, label, results):
    fractal = "benchmark_fractal"
    output = "benchmark_r_neighbors_nprocs"

    generate_fractal(rows=length, cols=length, fname=fractal)
    module = Module(
        "r.neighbors",
        input=fractal,
        output=output,
        size=7,
        memory=300,
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
