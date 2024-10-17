"""Benchmarking of r.slope aspect

@author Aaron Saw Min Sern
@author Anna Petrasova
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    reference = "r_slope_aspect_reference_map"
    generate_map(rows=10000, cols=10000, fname=reference)
    # Users can add more or modify existing reference maps
    benchmark(0, "r.slope.aspect_0MB", results, reference)
    benchmark(5, "r.slope.aspect_5MB", results, reference)
    benchmark(10, "r.slope.aspect_10MB", results, reference)
    benchmark(100, "r.slope.aspect_100MB", results, reference)
    benchmark(300, "r.slope.aspect_300MB", results, reference)

    Module("g.remove", quiet=True, flags="f", type="raster", name=reference)
    bm.nprocs_plot(results, filename="r_slope_aspect_benchmark_memory.svg")


def benchmark(memory, label, results, reference):
    slope = "benchmark_slope"
    aspect = "benchmark_aspect"
    pcurv = "benchmark_pcurv"
    tcurv = "benchmark_tcurv"

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
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=20, repeat=10))

    Module(
        "g.remove",
        quiet=True,
        flags="f",
        type="raster",
        name=(slope, aspect, pcurv, tcurv),
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
