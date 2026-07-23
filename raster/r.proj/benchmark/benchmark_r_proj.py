"""Benchmarking of r.proj thread scaling
raster (2D)

This follows the r.param.scale benchmark structure, sweeping raster size at a
fixed memory and then memory at a fixed raster size, and plotting the time,
speedup, and efficiency metrics with grass.benchmark. r.proj sweeps its compute
thread count through the nprocs= option. Each cell generates a source raster in
an EPSG:4326 project and reprojects it into EPSG:3857 in a temporary database,
so the script is self-contained. Run it with
grass --exec python benchmark_r_proj.py or from any GRASS session.
"""

import os
import tempfile

from grass.exceptions import CalledModuleError, GrassError
from grass.pygrass.modules import Module
import grass.script as gs
import grass.benchmark as bm

# Baselines held fixed while one dimension is swept.
BASE_MAPSIZE = 50e6  # cells
BASE_MEMORY = 300  # MB
MAPSIZES = [10e6, 50e6, 100e6]
MEMORIES = [50, 100, 300, 1000]
METRICS = ["time", "speedup", "efficiency"]
MAX_NPROCS = 8
REPEAT = 3

SRC_PROJECT = "src4326"
DST_PROJECT = "dst3857"
INPUT = "benchmark_r_proj_reference"
OUTPUT = "benchmark_r_proj"


def main():
    gisdbase = tempfile.mkdtemp(prefix="bench_r_proj_")
    gs.create_project(os.path.join(gisdbase, SRC_PROJECT), epsg="4326")
    gs.create_project(os.path.join(gisdbase, DST_PROJECT), epsg="3857")

    # Sweep raster size at the baseline memory.
    results = []
    for mapsize in MAPSIZES:
        benchmark(
            gisdbase,
            size=int(mapsize**0.5),
            memory=BASE_MEMORY,
            label=f"r.proj_{int(mapsize / 1e6)}M",
            results=results,
        )
    plot(results, "rastersize")

    # Sweep memory at the baseline raster size.
    results = []
    for memory in MEMORIES:
        benchmark(
            gisdbase,
            size=int(BASE_MAPSIZE**0.5),
            memory=memory,
            label=f"r.proj_memory_{memory}MB",
            results=results,
        )
    plot(results, "memory")


def benchmark(gisdbase, size, memory, label, results):
    generate_input(gisdbase, size)
    with gs.setup.init(
        os.path.join(gisdbase, DST_PROJECT), env=os.environ.copy()
    ) as session:
        env = session.env
        # Output region from r.proj's own suggested bounds for this input.
        text = gs.read_command(
            "r.proj",
            project=SRC_PROJECT,
            mapset="PERMANENT",
            dbase=gisdbase,
            input=INPUT,
            method="nearest",
            flags="g",
            env=env,
        )
        region = dict(token.split("=") for token in text.split())
        gs.run_command("g.region", env=env, **region)

        module = Module(
            "r.proj",
            project=SRC_PROJECT,
            mapset="PERMANENT",
            dbase=gisdbase,
            input=INPUT,
            output=OUTPUT,
            method="nearest",
            memory=memory,
            env_=env,
            run_=False,
            overwrite=True,
        )
        results.append(
            bm.benchmark_nprocs(
                module, label=label, max_nprocs=MAX_NPROCS, repeat=REPEAT
            )
        )


def generate_input(gisdbase, size):
    """Generate the size by size source raster in the EPSG:4326 project,
    mirroring the r.param.scale benchmark by trying r.surf.fractal and falling
    back to r.random.surface when fractal is unavailable, for example in a build
    without FFTW."""
    with gs.setup.init(
        os.path.join(gisdbase, SRC_PROJECT), env=os.environ.copy()
    ) as session:
        env = session.env
        gs.run_command(
            "g.region", n=50, s=40, w=-110, e=-90, rows=size, cols=size, env=env
        )
        try:
            Module("r.surf.fractal", output=INPUT, overwrite=True, env_=env)
        except (CalledModuleError, GrassError):
            Module("r.random.surface", output=INPUT, overwrite=True, env_=env)


def plot(results, sweep):
    for metric in METRICS:
        bm.nprocs_plot(
            results,
            filename=f"r_proj_{sweep}_{metric}.svg",
            title=f"r.proj {sweep} {metric}",
            metric=metric,
        )


if __name__ == "__main__":
    main()
