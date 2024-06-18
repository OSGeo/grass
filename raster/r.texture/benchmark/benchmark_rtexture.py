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
    mapsizes = [1e6, 2e6, 4e6, 8e6]

    for mapsize in mapsizes:
        benchmark(int(mapsize**0.5), f"r.texture_{int(mapsize/1e6)}M", results)

    bm.nprocs_plot(
        results,
        title="r.texture -a runtime",
        metric="speedup",
    )
    bm.nprocs_plot(
        results,
        title="r.texture -a runtime",
        metric="efficiency",
    )


def benchmark(size, label, results):
    reference = "r_texture_reference_map"
    basenames = [
        "ASM",
        "Contr",
        "Corr",
        "Var",
        "IDM",
        "SA",
        "SV",
        "SE",
        "Entr",
        "DV",
        "DE",
        "MOC-1",
        "MOC-2",
    ]

    generate_map(rows=size, cols=size, fname=reference)
    module = Module(
        "r.texture",
        input=reference,
        output="a",
        a=True,
        run_=False,
        stdout_=DEVNULL,
        overwrite=True,
    )
    results.append(bm.benchmark_nprocs(module, label=label, max_nprocs=8, repeat=3))
    Module("g.remove", quiet=True, flags="f", type="raster", name=reference)

    for basename in basenames:
        Module("g.remove", quiet=True, flags="f", type="raster", name=f"a_{basename}")


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
