"""Benchmarking of v.surf.rst

@author Aaron Saw Min Sern
        Chung-Yuan Liang
"""

from grass.pygrass.modules import Module
import grass.benchmark as bm
from subprocess import DEVNULL
import math


def main():
    results = []
    # Users can add more or modify existing reference maps
    raster_cells = [1e6, 2e6, 4e6, 8e6]  # 1M, 2M, 4M, 8M cells
    for ncells in raster_cells:
        benchmark(math.sqrt(ncells), f"v.surf.rst_{int(ncells / 1e6)}M", results)
    bm.nprocs_plot(results)


def benchmark(size, label, results):
    reference = "v_surf_rst_reference_map"
    output = "benchmark_v_surf_rst_nprocs"

    npoints = size * size * 0.1
    generate_map(npoints=npoints, rows=size, cols=size, fname=reference)

    module = Module(
        "v.surf.rst",
        input=reference,
        npmin=100,
        elevation=output,
        stdout_=DEVNULL,
        overwrite=True,
    )

    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=8, repeat=3))
    Module("g.remove", quiet=True, flags="f", type="raster", name=(reference, output))


def generate_map(npoints, rows, cols, fname):
    Module("g.region", flags="p", rows=rows, cols=cols, res=1)
    print("Generating reference map using v.random...")
    Module("v.random", output=fname, npoints=npoints, overwrite=True)


if __name__ == "__main__":
    main()
