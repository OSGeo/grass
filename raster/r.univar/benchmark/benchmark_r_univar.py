"""Benchmarking of r.univar

@author Aaron Saw Min Sern
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    # Users can add more or modify existing reference maps
    benchmark(7071, "r.univar_50M", results)
    benchmark(10000, "r.univar_100M", results)
    benchmark(14142, "r.univar_200M", results)
    benchmark(20000, "r.univar_400M", results)

    bm.nprocs_plot(results)


def benchmark(size, label, results):
    reference = "r_univar_reference_map"
    output = "benchmark_r_univar_nprocs"

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.univar",
        map=reference,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=16, repeat=3))
    Module("g.remove", quiet=True, flags="f", type="raster", name=(reference, output))


def generate_map(rows, cols, fname):
    Module("g.region", flags="p", rows=rows, cols=cols, res=1)
    # Generate using r.random.surface if r.surf.fractal fails
    try:
        print("Generating reference map using r.surf.fractal...")
        Module("r.surf.fractal", output=fname, overwrite=True)
    except CalledModuleError:
        print("r.surf.fractal fails, using r.random.surface instead...")
        Module("r.random.surface", output=fname, overwrite=True)


if __name__ == "__main__":
    main()
