"""Benchmarking of r.neighbors

@author Anna Petrasova
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    reference = "r_neighbors_reference_map"
    generate_map(rows=10000, cols=10000, fname=reference)
    # Users can add more or modify existing reference maps
    benchmark(3, "r.neighbors_3x3", results, reference)
    benchmark(9, "r.neighbors_9x9", results, reference)
    benchmark(15, "r.neighbors_15x15", results, reference)
    benchmark(31, "r.neighbors_31x31", results, reference)

    Module("g.remove", quiet=True, flags="f", type="raster", name=reference)
    bm.nprocs_plot(results, filename="r_neighbors_benchmark_size.svg")


def benchmark(size, label, results, reference):
    output = "benchmark_r_neighbors_nprocs"

    module = Module(
        "r.neighbors",
        input=reference,
        output=output,
        size=size,
        memory=300,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=16, repeat=3))
    Module("g.remove", quiet=True, flags="f", type="raster", name=output)


def generate_map(rows, cols, fname):
    Module("g.region", flags="p", s=0, n=rows, w=0, e=cols, res=1)
    # Generate using r.random.surface if r.surf.fractal fails
    try:
        print("Generating reference map using r.surf.fractal...")
        Module("r.surf.fractal", output=fname)
    except CalledModuleError:
        print("r.surf.fractal fails, using r.random.surface instead...")
        Module("r.random.surface", output=fname)


if __name__ == "__main__":
    main()
