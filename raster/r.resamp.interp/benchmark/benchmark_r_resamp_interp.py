"""Benchmarking of r.resamp.interp

@author Aaron Saw Min Sern
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    # Users can add more or modify existing reference maps
    benchmark(7071, "r.resamp.interp_50M", results)
    benchmark(14142, "r.resamp.interp_200M", results)
    benchmark(10000, "r.resamp.interp_100M", results)
    benchmark(20000, "r.resamp.interp_400M", results)

    bm.nprocs_plot(results)


def benchmark(size, label, results):
    reference = "r_resamp_interp_reference_map"
    output = "benchmark_r_resamp_interp"

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.resamp.interp",
        input=reference,
        output=output,
        method="lanczos",
        memory=500,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=16, repeat=3))
    Module("g.remove", quiet=True, flags="f", type="raster", name=(reference, output))


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
