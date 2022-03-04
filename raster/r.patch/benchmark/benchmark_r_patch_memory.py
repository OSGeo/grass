"""Benchmarking of r.patch memory parameter

@author Aaron Saw Min Sern
@author Anna Petrasova
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    references = ["r_patch_reference_map_{}".format(i) for i in range(4)]
    generate_map(rows=5000, cols=5000, fname=references)
    # Users can add more or modify existing reference maps
    benchmark(0, "r.patch_0MB", references, results)
    benchmark(5, "r.patch_5MB", references, results)
    benchmark(10, "r.patch_10MB", references, results)
    benchmark(100, "r.patch_100MB", references, results)
    benchmark(300, "r.patch_300MB", references, results)
    for r in references:
        Module("g.remove", quiet=True, flags="f", type="raster", name=r)
    bm.nprocs_plot(results, filename="rpatch_benchmark_memory.svg")


def benchmark(memory, label, references, results):
    output = "benchmark_r_patch"

    module = Module(
        "r.patch",
        input=references,
        output=output,
        memory=memory,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=24, repeat=10))
    Module("g.remove", quiet=True, flags="f", type="raster", name=output)


def generate_map(rows, cols, fname):
    temp = "r_patch_reference_tmp"

    Module("g.region", flags="p", s=0, n=rows, w=0, e=cols, res=1)
    # Generate using r.random.surface if r.surf.fractal fails
    try:
        print("Generating reference map using r.surf.fractal...")
        for r in fname:
            Module(
                "r.surf.fractal",
                output=temp,
                overwrite=True,
            )
            # Make approximate half of the reference map to be null
            Module(
                "r.mapcalc",
                expression=f"{r} = if({temp}, {temp}, 0, null())",
                overwrite=True,
            )
    except CalledModuleError:
        print("r.surf.fractal fails, using r.random.surface instead...")
        for r in fname:
            Module(
                "r.random.surface",
                maximum=255,
                output=temp,
                overwrite=True,
            )
            Module(
                "r.mapcalc",
                expression=f"{r} = if({temp} - 128, {temp}, 0, null())",
                overwrite=True,
            )

    Module("g.remove", quiet=True, flags="f", type="raster", name=temp)


if __name__ == "__main__":
    main()
