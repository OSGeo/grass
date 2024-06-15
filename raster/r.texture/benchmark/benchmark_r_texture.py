"""Benchmarking of r.texture

@author Aaron Saw Min Sern
        Chung-Yuan Liang
"""

from grass.exceptions import CalledModuleError
from grass.pygrass.modules import Module
from subprocess import DEVNULL

import grass.benchmark as bm


def main():
    results = []

    # Users can add more or modify existing reference maps
    benchmark(7071, "r.texture_50M", results)
    # benchmark(10000, "r.texture_100M", results)
    # benchmark(14142, "r.texture_200M", results)
    # benchmark(20000, "r.texture_400M", results)

    bm.nprocs_plot(results)


def benchmark(size, label, results):
    reference = "r_texture_reference_map"
    output = "benchmark_r_texture_nprocs"
    method = "asm"
    basename = "ASM"
    module_output = f"{output}_{basename}"

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.texture",
        input=reference,
        output=output,
        method=method,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=8, repeat=3))
    Module("g.remove", quiet=True, flags="f", type="raster", name=reference)
    Module("g.remove", quiet=True, flags="f", type="raster", name=module_output)


def generate_map(rows, cols, fname):
    Module("g.region", flags="p", rows=rows, cols=cols, res=1)
    # Generate using r.random.surface if r.surf.fractal fails
    try:
        print("Generating reference map using r.surf.fractal...")
        Module("r.surf.fractal", output=fname, overwrite=True)
    except CalledModuleError:
        print("r.surf.fractal fails, using r.random.surface instead...")
        Module("r.random.surface", output=fname, overwrite=True)


if __name__ == "__main__":
    main()
