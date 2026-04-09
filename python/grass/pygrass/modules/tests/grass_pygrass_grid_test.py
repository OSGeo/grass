"""Test main functions of PyGRASS GridModule"""

import functools
import multiprocessing
import sys
import pytest

import grass.script as gs


def _run_grid_module(module_name, project, run_kwargs=None, **kwargs):
    if run_kwargs is None:
        run_kwargs = {}

    import sys
    import grass.script as gs

    # Initialize GRASS in subprocess
    gs.setup.init(project)

    from grass.pygrass.modules.grid.grid import GridModule

    try:
        grid = GridModule(module_name, **kwargs)
        grid.run(**run_kwargs)
    except Exception as e:
        print(f"GridModule failed: {e}", file=sys.stderr)
        sys.exit(1)


def max_processes():
    """Get max useful number of parallel processes to run"""
    return min(multiprocessing.cpu_count(), 4)


# GridModule uses C libraries which can easily initialize only once
# and thus can't easily change location/mapset, so we use a subprocess
# to separate individual GridModule calls.


def run_in_subprocess(function, check=True):
    """Run function in a separate process"""
    ctx = multiprocessing.get_context("spawn")
    ctx.set_executable(sys.executable)  # force user's Python, not GRASS's
    process = ctx.Process(target=function)
    process.start()
    process.join()
    if check and process.exitcode != 0:
        msg = f"Subprocess failed with exit code {process.exitcode}"
        raise RuntimeError(msg)
    return process.exitcode


@pytest.mark.needs_solo_run
@pytest.mark.parametrize("processes", list(range(1, max_processes() + 1)) + [None])
def test_processes(tmp_path, processes):
    """Check that running with multiple processes works"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)

        surface = "surface"
        gs.run_command("r.surf.fractal", output=surface)

        run_in_subprocess(
            functools.partial(
                _run_grid_module,
                "r.slope.aspect",
                project=project,
                width=10,
                height=5,
                overlap=2,
                processes=processes,
                elevation=surface,
                slope="slope",
                aspect="aspect",
            )
        )

        info = gs.raster_info("slope")
        assert info["min"] > 0


# @pytest.mark.parametrize("split", [False])  # True does not work.


@pytest.mark.parametrize("width", [5, 10, 50])  # None does not work.
@pytest.mark.parametrize("height", [5, 10, 50])
def test_tiling_schemes(tmp_path, width, height):
    """Check that different shapes of tiles work"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)

        surface = "surface"
        gs.run_command("r.surf.fractal", output=surface)

        run_in_subprocess(
            functools.partial(
                _run_grid_module,
                "r.slope.aspect",
                project=project,
                width=width,
                height=height,
                overlap=2,
                processes=max_processes(),
                elevation=surface,
                slope="slope",
                aspect="aspect",
            )
        )

        info = gs.raster_info("slope")
        assert info["min"] > 0


@pytest.mark.parametrize("overlap", [0, 1, 2, 5])
def test_overlaps(tmp_path, overlap):
    """Check that overlap accepts different values"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)
        surface = "surface"
        gs.run_command("r.surf.fractal", output=surface)

        run_in_subprocess(
            functools.partial(
                _run_grid_module,
                "r.slope.aspect",
                project=project,
                width=10,
                height=5,
                overlap=overlap,
                processes=max_processes(),
                elevation=surface,
                slope="slope",
                aspect="aspect",
            )
        )

        info = gs.raster_info("slope")
        assert info["min"] > 0


@pytest.mark.parametrize("clean", [True, False])
@pytest.mark.parametrize("surface", ["surface", "non_exist_surface"])
def test_cleans(tmp_path, clean, surface):
    """Check that temporary mapsets are cleaned when appropriate"""
    project = tmp_path / "test"
    mapset_prefix = "abc"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)
        if surface == "surface":
            gs.run_command("r.surf.fractal", output=surface)

        run_in_subprocess(
            functools.partial(
                _run_grid_module,
                "r.slope.aspect",
                project=project,
                run_kwargs={"clean": clean},
                width=10,
                height=5,
                overlap=0,
                processes=max_processes(),
                elevation=surface,
                slope="slope",
                aspect="aspect",
                mapset_prefix=mapset_prefix,
            ),
            check=multiprocessing.get_start_method() == "spawn"
            or surface != "non_exist_surface",
        )

        prefixed = 0
        for item in project.iterdir():
            if item.is_dir():
                if clean:
                    # We know right away something is wrong.
                    assert not item.name.startswith(mapset_prefix), "Mapset not cleaned"
                else:
                    # We need to see if there is at least one prefixed mapset.
                    prefixed += int(item.name.startswith(mapset_prefix))
        if not clean:
            assert prefixed, "Not even one prefixed mapset"


@pytest.mark.parametrize("patch_backend", [None, "r.patch", "RasterRow"])
def test_patching_backend(tmp_path, patch_backend):
    """Check patching backend works"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)

        points = "points"
        reference = "reference"
        gs.run_command("v.random", output=points, npoints=100)
        gs.run_command(
            "v.to.rast", input=points, output=reference, type="point", use="cat"
        )

        run_in_subprocess(
            functools.partial(
                _run_grid_module,
                "v.to.rast",
                project=project,
                width=10,
                height=5,
                overlap=0,
                patch_backend=patch_backend,
                processes=max_processes(),
                input=points,
                output="output",
                type="point",
                use="cat",
            )
        )

        mean_ref = float(gs.parse_command("r.univar", map=reference, flags="g")["mean"])
        mean = float(gs.parse_command("r.univar", map="output", flags="g")["mean"])
        assert abs(mean - mean_ref) < 0.0001


@pytest.mark.parametrize(
    ("width", "height", "processes"),
    [
        (None, None, max_processes()),
        (10, None, max_processes()),
        (None, 5, max_processes()),
    ],
)
def test_tiling(tmp_path, width, height, processes):
    """Check auto adjusted tile size based on processes"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)

        surface = "surface"
        gs.run_command("r.surf.fractal", output=surface)

        run_in_subprocess(
            functools.partial(
                _run_grid_module,
                "r.slope.aspect",
                project=project,
                width=width,
                height=height,
                overlap=2,
                processes=processes,
                elevation=surface,
                slope="slope",
                aspect="aspect",
            )
        )

        info = gs.raster_info("slope")
        assert info["min"] > 0


@pytest.mark.needs_solo_run
@pytest.mark.parametrize(
    ("processes", "backend"),
    [
        (1, "RasterRow"),
        (9, "RasterRow"),
        (9, "r.patch"),
        (10, "RasterRow"),
        (10, "r.patch"),
    ],
)
def test_patching_error(tmp_path, processes, backend):
    """Check auto adjusted tile size based on processes"""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project):
        gs.run_command("g.region", s=0, n=10, w=0, e=10, res=0.1)
        surface = "fractal"

        run_in_subprocess(
            functools.partial(
                _run_grid_module,
                "r.surf.fractal",
                project=project,
                overlap=0,
                processes=processes,
                output=surface,
                patch_backend=backend,
                debug=True,
            )
        )

        info = gs.parse_command("r.univar", flags="g", map=surface)
        assert int(info["null_cells"]) == 0
