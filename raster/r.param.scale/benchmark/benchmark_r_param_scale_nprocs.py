"""Benchmarking of r.param.scale

@author Kaushik Raja
"""

import os
from subprocess import DEVNULL

import grass.benchmark as bm
from grass.exceptions import CalledModuleError, GrassError
from grass.pygrass.modules import Module


def main():
    results = []

    # Users can add more or modify existing reference maps
    benchmark(10000, "r.param.scale_100M", results)

    bm.nprocs_plot(results, filename="r_param_scale_benchmark_nprocs.svg")


def benchmark(size, label, results):
    reference = "r_param_scale_reference_map"
    output = "benchmark_r_param_scale_nprocs"

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.param.scale",
        input=reference,
        output=output,
        method="elev",
        size=31,
        memory=300,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(
        bm.benchmark_nprocs(module, label=label, max_nprocs=os.cpu_count(), repeat=3)
    )
    Module("g.remove", quiet=True, flags="f", type="raster", name=(reference, output))


def generate_map(rows, cols, fname):
    Module("g.region", flags="p", s=0, n=rows, w=0, e=cols, res=1)
    # Generate using r.random.surface if r.surf.fractal fails
    try:
        print("Generating reference map using r.surf.fractal...")
        Module("r.surf.fractal", output=fname)
    except (CalledModuleError, GrassError):
        print("r.surf.fractal fails, using r.random.surface instead...")
        Module("r.random.surface", output=fname)


if __name__ == "__main__":
    main()
