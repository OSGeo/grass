"""Benchmarking of r.patch

@author Aaron Saw Min Sern
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    # Users can add more or modify existing reference maps
    benchmark(7071, "r.patch_50M", results)
    benchmark(14142, "r.patch_200M", results)
    benchmark(10000, "r.patch_100M", results)
    benchmark(20000, "r.patch_400M", results)

    bm.nprocs_plot(results, filename="rpatch_benchmark.svg")


def benchmark(size, label, results):
    references = ["r_patch_reference_map_{}".format(i) for i in range(4)]
    output = "benchmark_r_patch"

    generate_map(rows=size, cols=size, fname=references)
    module = Module(
        "r.patch",
        input=references,
        output=output,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=24, repeat=3))
    for r in references:
        Module("g.remove", quiet=True, flags="f", type="raster", name=r)
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
<<<<<<< HEAD
<<<<<<< HEAD
                high=255,
=======
                maximum=255,
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
                maximum=255,
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
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
