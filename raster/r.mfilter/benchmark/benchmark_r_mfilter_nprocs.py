"""Benchmarking of r.mfilter

@author Aaron Saw Min Sern
"""

from pathlib import Path

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from grass.script import tempfile
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    # Users can add more or modify existing reference maps
    benchmark(7071, "r.mfilter_50M", results)
    benchmark(10000, "r.mfilter_100M", results)
    benchmark(14142, "r.mfilter_200M", results)
    benchmark(20000, "r.mfilter_400M", results)

    bm.nprocs_plot(results)


def benchmark(size, label, results):
    reference = "r_mfilter_reference_map"
    output = "benchmark_r_mfilter_nprocs"
    filter = tempfile()
    Path(filter).write_text(
        """MATRIX 9
                   1 1 1 1 1 1 1 1 1
                   1 2 1 2 1 2 1 2 1
                   1 1 3 1 3 1 3 1 1
                   1 2 1 4 1 4 1 2 1
                   1 1 3 1 5 1 3 1 1
                   1 2 1 4 1 4 1 2 1
                   1 1 3 1 3 1 3 1 1
                   1 2 1 2 1 2 1 2 1
                   1 1 1 1 1 1 1 1 1
                   DIVISOR 81
                   TYPE    P"""
    )

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.mfilter",
        input=reference,
        output=output,
        filter=filter,
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
