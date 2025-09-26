"""Benchmarking of r.slope aspect

@author Aaron Saw Min Sern
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    # Users can add more or modify existing reference maps
    benchmark(7071, "r.slope.aspect_50M", results)
    benchmark(10000, "r.slope.aspect_100M", results)
    benchmark(14142, "r.slope.aspect_200M", results)
    benchmark(20000, "r.slope.aspect_400M", results)

    bm.nprocs_plot(results, filename="r_slope_aspect_benchmark_size.svg")


def benchmark(size, label, results):
    reference = "r_slope_aspect_reference_map"
    slope = "benchmark_slope"
    aspect = "benchmark_aspect"
    pcurv = "benchmark_pcurv"
    tcurv = "benchmark_tcurv"

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.slope.aspect",
        elevation=reference,
        slope=slope,
        aspect=aspect,
        pcurvature=pcurv,
        tcurvature=tcurv,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=16, repeat=3))
    Module(
        "g.remove",
        quiet=True,
        flags="f",
        type="raster",
        name=(reference, slope, aspect, pcurv, tcurv),
    )


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
