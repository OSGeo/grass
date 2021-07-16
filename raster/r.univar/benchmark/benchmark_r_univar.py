"""Benchmarking of r.univar

@author Aaron Saw Min Sern
"""

from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    benchmark(7071, "r.univar_50M", results)
    benchmark(10000, "r.univar_100M", results)
    benchmark(14142, "r.univar_200M", results)
    benchmark(20000, "r.univar_400M", results)

    bm.nprocs_plot(results)


def benchmark(length, label, results):
    fractal = "benchmark_fractal"
    output = "benchmark_r_univar_nprocs"

    generate_fractal(rows=length, cols=length, fname=fractal)
    module = Module(
        "r.univar",
        map=[fractal] * 5,
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
