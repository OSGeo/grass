"""Benchmarking of r.neighbors

@author Aaron Saw Min Sern
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
    benchmark(0, "r.neighbors_0MB", results, reference)
    benchmark(5, "r.neighbors_5MB", results, reference)
    benchmark(10, "r.neighbors_10MB", results, reference)
    benchmark(100, "r.neighbors_100MB", results, reference)
    benchmark(300, "r.neighbors_300MB", results, reference)

    Module("g.remove", quiet=True, flags="f", type="raster", name=reference)
    bm.nprocs_plot(results, filename="r_neighbors_benchmark_memory.svg")


def benchmark(memory, label, results, reference):
    output = "benchmark_r_neighbors_nprocs"

    module = Module(
        "r.neighbors",
        input=reference,
        output=output,
        size=9,
        memory=memory,
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
